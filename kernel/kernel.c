#include "common.h"
#include "platform/platform.h"
#include "drivers/drivers.h"
#include "devices/devices.h"
#include "drivers_config.c"

[[noreturn]] void kernel_main(uintptr_t boot_param) {

    // Initialize platform-specific hardware
    platform_init();

    // Print hello world message
    puts("\n\n");
    puts("==================================\n");
    puts("       YasouOS v0.1.0\n");
    puts("==================================\n\n");
    puts("Hello World from YasouOS!\n");
    puts("Architecture: "ARCH_NAME"\n");

    // Get and print kernel command line
    const char *cmdline = platform_get_cmdline(boot_param);
    puts("Kernel command line: ");
    if (cmdline) {
        puts(cmdline);
    } else {
        puts("(none)");
    }
    puts("\n\n");

    // Register drivers
    drivers_config_register();

    // Scan device tree and count devices
    devices_scan();

    // Initialize devices with registered drivers
    drivers_init_devices();

    puts("\nSystem halted.\n");

    // Halt the system
    platform_halt();

    // Ensure we never return (in case platform_halt fails)
    for (;;) {
        __asm__ __volatile__("nop");
    }
}
