#include "common.h"
#include "log.h"
#include "platform/platform.h"
#include "devices/devices.h"
#include "resources/resources.h"
#include "init_apps.c"

static log_tag_t *kernel_log;

[[noreturn]] void kernel_main(uintptr_t boot_param) {

    // Initialize platform-specific hardware
    platform_init();

    // Print hello world message
    puts("\n\n");
    puts("==================================\n");
    puts("       YasouOS v0.1.0\n");
    puts("==================================\n\n");

    // Get kernel command line and initialize logging
    const char *cmdline = platform_get_cmdline(boot_param);
    log_init(cmdline);
    kernel_log = log_register("kernel", LOG_INFO);

    log_info(kernel_log, "Hello World from YasouOS!\n");
    log_info(kernel_log, "Architecture: "ARCH_NAME"\n");

    if (log_enabled(kernel_log, LOG_INFO)) {
        log_prefix(kernel_log, LOG_INFO);
        puts("Kernel command line: ");
        if (cmdline) {
            puts(cmdline);
        } else {
            puts("(none)");
        }
        puts("\n");
    }
    puts("\n");

    // Scan device tree and print
    device_set_fdt(boot_param);
    devices_scan();

    puts("\n\n");
    device_tree_print();

    // Initialize resource manager with device list
    resources_set_devices(devices_get_first());

    // Initialize applications based on command line
    puts("\n");
    init_apps(cmdline);

    log_info(kernel_log, "System halted.\n");

    // Halt the system
    platform_halt();

    // Ensure we never return (in case platform_halt fails)
    for (;;) {
        __asm__ __volatile__("nop");
    }
}
