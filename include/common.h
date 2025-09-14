#pragma once

#include "types.h"

// Common functions
void* memset(void* buf, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);

// I/O functions
void putchar(char ch);
void puts(const char* s);

// Platform-specific functions (must be implemented per arch)
void platform_init(void);
void platform_putchar(char ch);
void platform_puts(const char* s);
void platform_halt(void);