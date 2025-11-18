/*
 * Test Kernel Stubs
 * Stub implementations for kernel functions required by platform code
 */

#include "../../common/types.h"

// Forward declaration of test_kernel_main
void test_kernel_main(void);

// Stub for kernel_main - replaced by test_kernel_main
void kernel_main(void) {
    test_kernel_main();
}

// Provide putchar and puts implementations for the kernel platform
void putchar(char ch) {
    // Platform-specific serial output
    #if defined(__x86_64__)
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)ch), "Nd"((uint16_t)0x3F8));
    #elif defined(__aarch64__)
        *(volatile uint32_t *)0x09000000 = (uint32_t)ch;
    #elif defined(__riscv)
        *(volatile uint8_t *)0x10000000 = (uint8_t)ch;
    #endif
}

void puts(const char *str) {
    while (*str) {
        putchar(*str++);
    }
}

// Stubs for FDT functions (ARM64/RISC-V only)
const char *fdt_get_bootargs(void) {
    return "";
}

// Stubs for hex output functions (ARM64/RISC-V only)
void put_hex8(uint8_t val) {
    (void)val;
}

void put_hex32(uint32_t val) {
    (void)val;
}

void put_hex64(uint64_t val) {
    (void)val;
}

// Platform exit function
void platform_exit(int code) {
    #if defined(__x86_64__)
        // QEMU isa-debug-exit device
        __asm__ volatile("outw %0, %1" : : "a"((uint16_t)(code == 0 ? 0x2000 : 0x2001)), "Nd"((uint16_t)0xf4));
    #elif defined(__aarch64__)
        // ARM PSCI system_off
        (void)code;
        register uint64_t x0 __asm__("x0") = 0x84000008;  // PSCI SYSTEM_OFF
        __asm__ volatile("hvc #0" : : "r"(x0));
        while(1) { __asm__ volatile("wfe"); }
    #elif defined(__riscv)
        // RISC-V SBI shutdown
        (void)code;
        register uint64_t a0 __asm__("a0") = 0;
        register uint64_t a1 __asm__("a1") = 0;
        register uint64_t a6 __asm__("a6") = 0;
        register uint64_t a7 __asm__("a7") = 8;  // SBI SHUTDOWN
        __asm__ volatile("ecall" : : "r"(a0), "r"(a1), "r"(a6), "r"(a7));
        while(1) { __asm__ volatile("wfi"); }
    #endif
}
