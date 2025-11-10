#include "common.h"
#include "platform/platform.h"
#include "devices/devices.h"
#include "resources/resources.h"
#include "init_apps.c"

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

    // Scan device tree and print
    device_set_fdt(boot_param);
    devices_scan();
    device_tree_print();

    // Initialize resource manager with device list
    resources_set_devices(devices_get_first());

    // Initialize applications based on command line
    puts("\n");
    init_apps(cmdline);

    puts("\nSystem halted.\n");

    // Halt the system
    platform_halt();

    // Ensure we never return (in case platform_halt fails)
    for (;;) {
        __asm__ __volatile__("nop");
    }
}
