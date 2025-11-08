#include "common.h"
#include "../platform.h"

void platform_init(void)
{
    // QEMU virt machine PL011 UART is pre-initialized
}

void platform_putchar(char ch)
{
    // Direct PL011 UART access - QEMU virt machine UART at 0x09000000
    volatile uint32_t *uart = (volatile uint32_t *)0x09000000;
    *uart = ch;
}

void platform_puts(const char *str)
{
    while (*str)
    {
        platform_putchar(*str++);
    }
}

void platform_halt(void)
{
    // Try PSCI (Power State Coordination Interface) system shutdown first
    register long x0 __asm__("x0") = 0x84000008; // PSCI_SYSTEM_OFF
    __asm__ volatile("hvc #0" : : "r"(x0) : "memory");

    // Fallback: Use semihosting to exit QEMU cleanly
    long params[2] = {0x20026, 0}; // ADP_Stopped_ApplicationExit, exit code 0
    register long x0_semi __asm__("x0") = 0x18; // SYS_EXIT
    register long x1_semi __asm__("x1") = (long)params;
    __asm__ volatile("hlt #0xf000" : : "r"(x0_semi), "r"(x1_semi) : "memory");

    // Final fallback: infinite loop
    while (1)
    {
        __asm__ volatile("wfe");
    }
}