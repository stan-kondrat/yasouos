/*
 * Test Kernel Stubs
 * Minimal stubs for running unit tests as freestanding kernels.
 * I/O and string functions are provided by platform.c and common.c.
 */

#include "../../common/types.h"

// Forward declaration of test_kernel_main
void test_kernel_main(void);

// Redirect kernel_main to the test entry point
void kernel_main(void) {
    test_kernel_main();
}

// Stub for FDT bootargs (ARM64/RISC-V boot_kernel.S references this)
const char *fdt_get_bootargs(void) {
    return "";
}

// Platform exit — shuts down QEMU with an exit code
void platform_exit(int code) {
    #if defined(__x86_64__)
        __asm__ volatile("outw %0, %1" : : "a"((uint16_t)(code == 0 ? 0x2000 : 0x2001)), "Nd"((uint16_t)0xf4));
    #elif defined(__aarch64__)
        (void)code;
        register uint64_t x0 __asm__("x0") = 0x84000008;
        __asm__ volatile("hvc #0" : : "r"(x0));
        while(1) { __asm__ volatile("wfe"); }
    #elif defined(__riscv)
        (void)code;
        register uint64_t a0 __asm__("a0") = 0;
        register uint64_t a1 __asm__("a1") = 0;
        register uint64_t a6 __asm__("a6") = 0;
        register uint64_t a7 __asm__("a7") = 8;
        __asm__ volatile("ecall" : : "r"(a0), "r"(a1), "r"(a6), "r"(a7));
        while(1) { __asm__ volatile("wfi"); }
    #endif
}
