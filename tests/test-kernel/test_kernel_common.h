/*
 * Test Kernel Common Header
 * Common definitions and utilities for unit test kernels
 */

#ifndef TEST_KERNEL_COMMON_H
#define TEST_KERNEL_COMMON_H

#include "../../common/types.h"
#include "test_platform.h"

// Test result tracking (defined in test_kernel_common.c)
extern int tests_passed;
extern int tests_failed;

// Test framework functions
void test_suite_start(const char *suite_name);
void test_start(const char *test_name);
void test_assert_true(bool condition, const char *message);
void test_assert_eq_uint32(uint32_t actual, uint32_t expected, const char *message);
void test_assert_eq_uint16(uint16_t actual, uint16_t expected, const char *message);
void test_assert_mem_eq(const void *actual, const void *expected, size_t n, const char *message);
void test_suite_end(void);

// Helper functions (defined in test_kernel_common.c)
bool memcmp_simple(const void *s1, const void *s2, size_t n);
void print_number(int n);
void uint32_to_hex(uint32_t val, char *buf);
void uint16_to_hex(uint16_t val, char *buf);

#endif
