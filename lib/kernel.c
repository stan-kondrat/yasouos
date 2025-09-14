#include "common.h"

void kernel_main(void) {

    // Initialize platform-specific hardware
    platform_init();

    // Print hello world message
    puts("\n\n");
    puts("==================================\n");
    puts("       YasouOS v0.1.0\n");
    puts("==================================\n\n");
    puts("Hello World from YasouOS!\n");
    puts("Architecture: "ARCH_NAME"\n");
    puts("\nSystem halte.\n");

    // Halt the system
    platform_halt();
}