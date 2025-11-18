/*
 * Test Kernel Common Functions
 * Helper functions for unit testing in freestanding environment
 */

#include "test_kernel_common.h"

// Global test counters
int tests_passed = 0;
int tests_failed = 0;

// Current test context
static const char *current_test_suite = NULL;
static const char *current_test_name = NULL;

// Helper function for memory comparison
bool memcmp_simple(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return false;
        }
    }
    return true;
}

// Helper function to print numbers
void print_number(int n) {
    if (n < 0) {
        putchar('-');
        n = -n;
    }

    char buf[12];
    int i = 0;

    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
    }

    while (i > 0) {
        putchar(buf[--i]);
    }
}

// Helper functions to convert numbers to hex strings
void uint32_to_hex(uint32_t val, char *buf) {
    const char hex[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = hex[(val >> (28 - i * 4)) & 0xF];
    }
    buf[10] = '\0';
}

void uint16_to_hex(uint16_t val, char *buf) {
    const char hex[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 4; i++) {
        buf[2 + i] = hex[(val >> (12 - i * 4)) & 0xF];
    }
    buf[6] = '\0';
}

void test_suite_start(const char *suite_name) {
    current_test_suite = suite_name;
    puts("tests starts\n");
}

void test_start(const char *test_name) {
    current_test_name = test_name;
}

void test_assert_true(bool condition, const char *message) {
    if (condition) {
        tests_passed++;
        if (current_test_name) {
            puts("PASS: ");
            puts(current_test_name);
            if (message) {
                puts(" - ");
                puts(message);
            }
            puts("\n");
        }
    } else {
        tests_failed++;
        if (current_test_name) {
            puts("FAIL: ");
            puts(current_test_name);
            if (message) {
                puts(" - ");
                puts(message);
            }
            puts("\n");
        }
    }
}

void test_assert_eq_uint32(uint32_t actual, uint32_t expected, const char *message) {
    test_assert_true(actual == expected, message);
}

void test_assert_eq_uint16(uint16_t actual, uint16_t expected, const char *message) {
    test_assert_true(actual == expected, message);
}

void test_assert_mem_eq(const void *actual, const void *expected, size_t n, const char *message) {
    test_assert_true(memcmp_simple(actual, expected, n), message);
}

void test_suite_end(void) {
    puts("Tests passed: ");
    print_number(tests_passed);
    puts(", tests failed: ");
    print_number(tests_failed);
    puts("\n");
    platform_exit(tests_failed);
}
