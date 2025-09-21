#include "common.h"

[[noreturn]] void kernel_main(void) {

    // Initialize platform-specific hardware
    platform_init();

    // Print hello world message
    puts("\n\n");
    puts("==================================\n");
    puts("       YasouOS v0.1.0\n");
    puts("==================================\n\n");
    puts("Hello World from YasouOS!\n");
    puts("Architecture: "ARCH_NAME"\n");
    puts("\nSystem halted.\n");

    // Halt the system
    platform_halt();

    // Ensure we never return (in case platform_halt fails)
    for (;;) {
        __asm__ __volatile__("nop");
    }
}