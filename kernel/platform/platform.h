#pragma once

#include "../../common/types.h"

// Platform-specific functions (must be implemented per arch)
void platform_init(void);
void platform_putchar(char ch);
void platform_puts(const char* s);
void platform_halt(void);
const char* platform_get_cmdline(uintptr_t boot_param);
