#include "log.h"
#include "common.h"

#define MAX_LOG_TAGS 32

struct log_tag {
    const char *name;
    log_level_t level;
};

static log_tag_t tag_registry[MAX_LOG_TAGS];
static int tag_count = 0;
static log_level_t global_level = LOG_INFO;
static bool global_level_set = false;

static const char *level_names[] = {
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

// Parse a level string ("error", "warn", "info", "debug") to enum.
// Returns -1 on failure.
static int parse_level(const char *s, size_t len) {
    if (len == 5 && strncmp(s, "error", 5) == 0) return LOG_ERROR;
    if (len == 4 && strncmp(s, "warn", 4) == 0)  return LOG_WARN;
    if (len == 4 && strncmp(s, "info", 4) == 0)   return LOG_INFO;
    if (len == 5 && strncmp(s, "debug", 5) == 0) return LOG_DEBUG;
    return -1;
}

// Get the length of a cmdline token (until space, tab, or null)
static size_t token_len(const char *s) {
    size_t len = 0;
    while (s[len] && s[len] != ' ' && s[len] != '\t') len++;
    return len;
}

// Apply a per-tag override from cmdline. Called after all tags are registered
// when log_init runs, but tags may register after init too — so we store
// pending overrides and apply them on register as well.
//
// NOTE: override name pointers point into the cmdline string passed to
// log_init(). The caller must ensure that string outlives the logging system
// (e.g. FDT memory, bootloader data, or a string literal).
#define MAX_OVERRIDES 16
static struct {
    const char *name;
    size_t name_len;
    log_level_t level;
} pending_overrides[MAX_OVERRIDES];
static int override_count = 0;

static void apply_overrides(log_tag_t *tag) {
    for (int i = 0; i < override_count; i++) {
        if (strlen(tag->name) == pending_overrides[i].name_len &&
            strncmp(tag->name, pending_overrides[i].name, pending_overrides[i].name_len) == 0) {
            tag->level = pending_overrides[i].level;
            return;
        }
    }
}

void log_init(const char *cmdline) {
    if (!cmdline) return;

    const char *pos = cmdline;
    while (*pos) {
        // Skip whitespace
        while (*pos == ' ' || *pos == '\t') pos++;
        if (!*pos) break;

        // Check for "log=" (global level)
        if (strncmp(pos, "log=", 4) == 0 && pos[4] != '\0' &&
            pos[4] != ' ' && pos[4] != '\t') {
            const char *val = pos + 4;
            size_t vlen = token_len(val);
            int lvl = parse_level(val, vlen);
            if (lvl >= 0) {
                global_level = (log_level_t)lvl;
                global_level_set = true;
                // Update already-registered tags, then re-apply per-tag overrides
                for (int i = 0; i < tag_count; i++) {
                    tag_registry[i].level = global_level;
                    apply_overrides(&tag_registry[i]);
                }
            }
        }

        // Check for "log.<tag>=<level>"
        if (strncmp(pos, "log.", 4) == 0) {
            const char *tag_start = pos + 4;
            const char *eq = tag_start;
            while (*eq && *eq != '=' && *eq != ' ' && *eq != '\t') eq++;
            if (*eq == '=' && eq > tag_start) {
                size_t name_len = eq - tag_start;
                const char *val = eq + 1;
                size_t vlen = token_len(val);
                int lvl = parse_level(val, vlen);
                if (lvl >= 0 && override_count < MAX_OVERRIDES) {
                    pending_overrides[override_count].name = tag_start;
                    pending_overrides[override_count].name_len = name_len;
                    pending_overrides[override_count].level = (log_level_t)lvl;
                    override_count++;

                    // Apply to already-registered tags
                    for (int i = 0; i < tag_count; i++) {
                        if (strlen(tag_registry[i].name) == name_len &&
                            strncmp(tag_registry[i].name, tag_start, name_len) == 0) {
                            tag_registry[i].level = (log_level_t)lvl;
                        }
                    }
                }
            }
        }

        // Skip to next token
        while (*pos && *pos != ' ' && *pos != '\t') pos++;
    }
}

log_tag_t *log_register(const char *name, log_level_t default_level) {
    if (tag_count >= MAX_LOG_TAGS) return NULL;

    log_tag_t *tag = &tag_registry[tag_count++];
    tag->name = name;
    tag->level = global_level_set ? global_level : default_level;

    // Apply any per-tag overrides from cmdline
    apply_overrides(tag);

    return tag;
}

bool log_enabled(const log_tag_t *tag, log_level_t level) {
    if (!tag) return false;
    return level <= tag->level;
}

void log_prefix(const log_tag_t *tag, log_level_t level) {
    if (!tag || level > LOG_DEBUG) return;
    puts("[");
    puts(level_names[level]);
    puts("][");
    puts(tag->name);
    puts("] ");
}

static void log_msg(const log_tag_t *tag, log_level_t level, const char *msg) {
    if (!log_enabled(tag, level)) return;
    log_prefix(tag, level);
    puts(msg);
}

void log_error(const log_tag_t *tag, const char *msg) { log_msg(tag, LOG_ERROR, msg); }
void log_warn(const log_tag_t *tag, const char *msg)  { log_msg(tag, LOG_WARN, msg); }
void log_info(const log_tag_t *tag, const char *msg)   { log_msg(tag, LOG_INFO, msg); }
void log_debug(const log_tag_t *tag, const char *msg) { log_msg(tag, LOG_DEBUG, msg); }
