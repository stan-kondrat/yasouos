/*
 * Log System Test Suite (Freestanding)
 *
 * Tests run sequentially. Since log.c has static global state (no reset),
 * each test accounts for state left by previous tests.
 *
 * State trace:
 *   start:           global=INFO, tags=0, overrides=0
 *   test_default:    global=INFO, tags=1
 *   test_null:       global=INFO, tags=1
 *   test_global_dbg: global=DEBUG, tags=2
 *   test_global_err: global=ERROR, tags=3
 *   test_per_tag:    global=ERROR, tags=5, overrides=1 (verbose=DEBUG)
 *   test_mixed:      global=WARN, tags=7, overrides=2 (+driver=DEBUG)
 *   test_retro:      global=WARN, tags=8, overrides=3 (+retro=DEBUG)
 *   test_other:      global=INFO, tags=9
 *   test_invalid:    global=INFO, tags=10
 *   test_prefix:     global=INFO, tags=11
 *   test_edge:       global=INFO, tags<=14
 */

#include "../tests/test-kernel/test_kernel_common.h"
#include "log.h"

// --- Before any log_init (default global = LOG_INFO) ---

void test_default_level(void) {
    test_start("default level is INFO");
    log_tag_t *tag = log_register("default-test", LOG_INFO);
    test_assert_true(tag != NULL, "register returns non-NULL");
    test_assert_true(log_enabled(tag, LOG_ERROR), "ERROR enabled at INFO");
    test_assert_true(log_enabled(tag, LOG_WARN), "WARN enabled at INFO");
    test_assert_true(log_enabled(tag, LOG_INFO), "INFO enabled at INFO");
    test_assert_true(!log_enabled(tag, LOG_DEBUG), "DEBUG disabled at INFO");
}

void test_null_safety(void) {
    test_start("null safety");
    test_assert_true(!log_enabled(NULL, LOG_INFO), "null tag returns false");
    log_init(NULL);
    log_prefix(NULL, LOG_INFO);
    log_error(NULL, "test");
    log_warn(NULL, "test");
    log_info(NULL, "test");
    log_debug(NULL, "test");
    test_assert_true(true, "null args do not crash");
}

// --- log_init with global level ---

void test_global_level_debug(void) {
    test_start("global level debug");
    log_init("log=debug");
    log_tag_t *tag = log_register("debug-test", LOG_INFO);
    test_assert_true(tag != NULL, "register non-NULL");
    test_assert_true(log_enabled(tag, LOG_ERROR), "ERROR enabled");
    test_assert_true(log_enabled(tag, LOG_WARN), "WARN enabled");
    test_assert_true(log_enabled(tag, LOG_INFO), "INFO enabled");
    test_assert_true(log_enabled(tag, LOG_DEBUG), "DEBUG enabled after log=debug");
}

void test_global_level_error(void) {
    test_start("global level error");
    log_init("log=error");
    log_tag_t *tag = log_register("error-test", LOG_INFO);
    test_assert_true(log_enabled(tag, LOG_ERROR), "ERROR enabled after log=error");
    test_assert_true(!log_enabled(tag, LOG_WARN), "WARN disabled after log=error");
    test_assert_true(!log_enabled(tag, LOG_INFO), "INFO disabled after log=error");
    test_assert_true(!log_enabled(tag, LOG_DEBUG), "DEBUG disabled after log=error");
}

// --- Per-tag overrides (global is ERROR from above) ---

void test_per_tag_override(void) {
    test_start("per-tag override");
    log_init("log.verbose=debug");
    log_tag_t *verbose = log_register("verbose", LOG_INFO);
    log_tag_t *quiet = log_register("quiet", LOG_INFO);

    test_assert_true(log_enabled(verbose, LOG_DEBUG), "overridden tag at DEBUG");
    test_assert_true(log_enabled(quiet, LOG_ERROR), "non-overridden tag at ERROR (global)");
    test_assert_true(!log_enabled(quiet, LOG_WARN), "non-overridden tag WARN disabled");
}

// --- Mixed cmdline: global + per-tag ---

void test_mixed_cmdline(void) {
    test_start("mixed global and per-tag cmdline");
    log_init("log=warn log.driver=debug");
    log_tag_t *driver = log_register("driver", LOG_INFO);
    log_tag_t *app = log_register("app", LOG_INFO);

    test_assert_true(log_enabled(driver, LOG_DEBUG), "driver at DEBUG (override)");
    test_assert_true(log_enabled(app, LOG_WARN), "app at WARN (global)");
    test_assert_true(!log_enabled(app, LOG_INFO), "app INFO disabled");
}

// --- Retroactive per-tag override (global is WARN from above) ---

void test_retroactive_override(void) {
    test_start("retroactive per-tag override");
    log_tag_t *retro = log_register("retro", LOG_INFO);
    // retro gets WARN (current global)
    test_assert_true(log_enabled(retro, LOG_WARN), "before override: WARN enabled");
    test_assert_true(!log_enabled(retro, LOG_DEBUG), "before override: DEBUG disabled");

    log_init("log.retro=debug");
    // retroactive override should set retro to DEBUG
    test_assert_true(log_enabled(retro, LOG_DEBUG), "after override: DEBUG enabled");
}

// --- Cmdline with non-log params ---

void test_cmdline_with_other_params(void) {
    test_start("cmdline with non-log params");
    log_init("app=http-hello log=info console=ttyS0");
    log_tag_t *tag = log_register("mixed-params", LOG_INFO);
    test_assert_true(log_enabled(tag, LOG_INFO), "INFO enabled");
    test_assert_true(!log_enabled(tag, LOG_DEBUG), "DEBUG disabled");
}

// --- Invalid level string ---

void test_invalid_level(void) {
    test_start("invalid level string");
    log_init("log=invalid");
    // "invalid" doesn't parse; global stays INFO from previous test
    log_tag_t *tag = log_register("invalid-test", LOG_INFO);
    test_assert_true(log_enabled(tag, LOG_INFO), "INFO still enabled");
    test_assert_true(!log_enabled(tag, LOG_DEBUG), "DEBUG still disabled");
}

// --- Prefix and convenience functions don't crash ---

void test_prefix_output(void) {
    test_start("prefix and convenience functions");
    log_tag_t *tag = log_register("prefix-test", LOG_INFO);
    log_prefix(tag, LOG_ERROR);
    log_prefix(tag, LOG_WARN);
    log_prefix(tag, LOG_INFO);
    log_prefix(tag, LOG_DEBUG);
    log_error(tag, "err\n");
    log_warn(tag, "wrn\n");
    log_info(tag, "inf\n");
    log_debug(tag, "dbg\n");  // should be suppressed (level=INFO)
    test_assert_true(true, "prefix and log functions work");
}

// --- Edge cases ---

void test_edge_cases(void) {
    test_start("edge cases");

    // Empty cmdline
    log_init("");
    test_assert_true(true, "empty cmdline ok");

    // Whitespace-only cmdline
    log_init("   ");
    test_assert_true(true, "whitespace cmdline ok");

    // log= with no value (should not crash or match)
    log_init("log= other=thing");
    log_tag_t *tag = log_register("edge1", LOG_INFO);
    test_assert_true(tag != NULL, "register after bare log= ok");

    // log. with no tag name (log.=debug)
    log_init("log.=debug");
    log_tag_t *tag2 = log_register("edge2", LOG_INFO);
    test_assert_true(tag2 != NULL, "register after log.=debug ok");

    // Partial match: "logger=foo" should not match "log="
    log_init("logger=debug");
    log_tag_t *tag3 = log_register("edge3", LOG_INFO);
    test_assert_true(log_enabled(tag3, LOG_INFO), "logger= does not match log=");
}

void test_kernel_main(void) {
    test_suite_start("Log System");

    test_default_level();
    test_null_safety();
    test_global_level_debug();
    test_global_level_error();
    test_per_tag_override();
    test_mixed_cmdline();
    test_retroactive_override();
    test_cmdline_with_other_params();
    test_invalid_level();
    test_prefix_output();
    test_edge_cases();

    test_suite_end();
}
