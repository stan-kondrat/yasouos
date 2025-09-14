#include "common.h"

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