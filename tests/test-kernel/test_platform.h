/*
 * Test Platform Header
 * Platform-specific I/O functions for test kernels
 */

#ifndef TEST_PLATFORM_H
#define TEST_PLATFORM_H

// Platform-specific I/O functions (implemented in test_platform.*.c)
void platform_init(void);
void putchar(char ch);
void puts(const char *str);
void platform_exit(int code);

#endif
