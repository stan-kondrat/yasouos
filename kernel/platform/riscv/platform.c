#include "common.h"
#include "../platform.h"
#include "../fdt_parser.h"

void platform_init(void) {
    // Platform initialization complete - using direct hardware access
}

void platform_putchar(char ch) {
    // Direct UART access - QEMU virt machine UART at 0x10000000
    volatile uint8_t *uart = (volatile uint8_t *)0x10000000;
    *uart = ch;
}

void platform_puts(const char* str) {
    while (*str) {
        platform_putchar(*str++);
    }
}

void platform_halt(void) {
    // QEMU test device shutdown
    volatile uint32_t *test_device = (volatile uint32_t *)0x100000;
    *test_device = 0x5555;
}

const char* platform_get_cmdline(uintptr_t boot_param) {
    // Use FDT parser to extract bootargs from device tree
    return fdt_get_bootargs(boot_param);
}