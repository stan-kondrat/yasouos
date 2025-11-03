#pragma once

// Platform-specific functions (must be implemented per arch)
void platform_init(void);
void platform_putchar(char ch);
void platform_puts(const char* s);
void platform_halt(void);
