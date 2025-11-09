#include "common.h"
#include "../platform.h"
#include "../fdt_parser.h"

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
    // Use PSCI (Power State Coordination Interface) system shutdown
    register long x0 __asm__("x0") = 0x84000008; // PSCI_SYSTEM_OFF
    __asm__ volatile("hvc #0" : : "r"(x0) : "memory");

    // Fallback: infinite loop
    while (1)
    {
        __asm__ volatile("wfe");
    }
}

const char* platform_get_cmdline(uintptr_t boot_param) {
    // Use FDT parser to extract bootargs from device tree
    return fdt_get_bootargs(boot_param);
}