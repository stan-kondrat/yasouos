#include "common.h"

// QEMU virt machine UART0 base address (PL011)
#define UART0_BASE 0x09000000

// UART registers
#define UART_DATA (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART_FR (*(volatile unsigned int *)(UART0_BASE + 0x18))


void test_semihosting(void)
{
    // SYS_EXIT - this should terminate QEMU if semihosting works
    long params[1] = {0}; // exit code 0
    register long x0 __asm__("x0") = 0x18; // SYS_EXIT
    register long x1 __asm__("x1") = (long)params;
    __asm__ volatile("hlt #0xf000" : : "r"(x0), "r"(x1) : "memory");
}


void platform_init(void)
{
    // For QEMU virt machine, UART is already initialized
    // test_semihosting();
}



void platform_putchar(char ch)
{
    // Use ARM semihosting for debugging
    register long x0 __asm__("x0") = 0x03; // SYS_WRITEC
    register long x1 __asm__("x1") = (long)&ch;
    __asm__ volatile ("hlt #0xf000" : : "r"(x0), "r"(x1) : "memory");
}

void platform_puts(const char *str)
{
    while (*str)
    {
        platform_putchar(*str++);
    }
    
    // if (!str) return;

    // // Calculate string length manually
    // long len = 0;
    // while (str[len]) len++;

    // long params[3] = {1, (long)str, len};
    // register long x0 __asm__("x0") = 0x05; // SYS_WRITE
    // register long x1 __asm__("x1") = (long)params;
    // __asm__ volatile("hlt #0xf000" : : "r"(x0), "r"(x1) : "memory");
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