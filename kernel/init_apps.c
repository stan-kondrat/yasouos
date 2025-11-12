// init_apps.c - Application initialization based on kernel command line
#include "../common/common.h"
#include "../apps/random/random.h"
#include "../drivers/virtio_net/virtio_net.h"
#include "../drivers/e1000/e1000.h"
#include "../drivers/rtl8139/rtl8139.h"
#include "resources/resources.h"

// Application prototypes
extern void app_illegal_instruction(void);

// VirtIO-Net device contexts (supports up to 4 network devices)
#define MAX_NET_DEVICES 4
static virtio_net_t virtio_net_contexts[MAX_NET_DEVICES] = {0};
static int virtio_net_device_count = 0;

// E1000 device contexts (supports up to 4 network devices)
static e1000_t e1000_contexts[MAX_NET_DEVICES] = {0};
static int e1000_device_count = 0;

// RTL8139 device contexts (supports up to 4 network devices)
static rtl8139_t rtl8139_contexts[MAX_NET_DEVICES] = {0};
static int rtl8139_device_count = 0;

// Forward declarations
static const char* find_param(const char* cmdline, const char* param);
static const char* find_next_param(const char* start, const char* param);
static int param_has_value(const char* param_pos, const char* value);

void init_apps(const char* cmdline) {
    if (cmdline == NULL) {
        return;
    }

    // Process all app= parameters in order
    const char* app_param = find_param(cmdline, "app");
    while (app_param != NULL) {
        // Check for app=illegal-instruction
        if (param_has_value(app_param, "illegal-instruction")) {
            app_illegal_instruction();
            // Should never return
        }

        // Check for app=random-software
        if (param_has_value(app_param, "random-software")) {
            uint8_t buffer[8];
            int result = random_get_bytes(buffer, sizeof(buffer));
            if (result > 0) {
                puts("Random (software): ");
                for (int i = 0; i < result; i++) {
                    put_hex8(buffer[i]);
                    puts(" ");
                }
                puts("\n");
            }
        }

        // Check for app=random-hardware
        if (param_has_value(app_param, "random-hardware")) {
            int hw_result = random_hardware_init();
            uint8_t buffer[8];
            int result = random_get_bytes(buffer, sizeof(buffer));
            if (result > 0) {
                if (hw_result == 0) {
                    puts("Random (hardware): ");
                } else {
                    puts("Random (software): ");
                }
                for (int i = 0; i < result; i++) {
                    put_hex8(buffer[i]);
                    puts(" ");
                }
                puts("\n");
            }
        }

        // Check for app=mac-virtio-net
        if (param_has_value(app_param, "mac-virtio-net")) {
            if (virtio_net_device_count >= MAX_NET_DEVICES) {
                puts("virtio-net: Maximum number of devices reached\n");
            } else {
                const driver_t *driver = virtio_net_get_driver();
                resource_t *resource = resource_acquire_available(driver, &virtio_net_contexts[virtio_net_device_count]);

                if (resource != NULL) {
                    resource_print_tag(resource);
                    puts(" Initializing...\n");

                    resource_print_tag(resource);
                    puts(" MAC: ");

                    uint8_t mac[6];
                    int result = virtio_net_get_mac(&virtio_net_contexts[virtio_net_device_count], mac);
                    if (result == 0) {
                        for (int i = 0; i < 6; i++) {
                            put_hex8(mac[i]);
                            if (i < 5) {
                                puts(":");
                            }
                        }
                    } else {
                        puts("(unavailable)");
                    }
                    puts("\n");

                    virtio_net_device_count++;
                } else {
                    puts("virtio-net: No available device\n");
                }
            }
        }

        // Check for app=mac-e1000
        if (param_has_value(app_param, "mac-e1000")) {
            if (e1000_device_count >= MAX_NET_DEVICES) {
                puts("e1000: Maximum number of devices reached\n");
            } else {
                const driver_t *driver = e1000_get_driver();
                if (!driver) {
                    puts("e1000: Failed to get driver\n");
                    app_param = find_next_param(app_param, "app");
                    continue;
                }

                resource_t *resource = resource_acquire_available(driver, &e1000_contexts[e1000_device_count]);

                if (resource != NULL) {
                    resource_print_tag(resource);
                    puts(" Initializing...\n");

                    resource_print_tag(resource);
                    puts(" MAC: ");

                    uint8_t mac[6];
                    int result = e1000_get_mac(&e1000_contexts[e1000_device_count], mac);
                    if (result == 0) {
                        for (int i = 0; i < 6; i++) {
                            put_hex8(mac[i]);
                            if (i < 5) {
                                puts(":");
                            }
                        }
                    } else {
                        puts("(unavailable)");
                    }
                    puts("\n");

                    e1000_device_count++;
                } else {
                    puts("e1000: No available device\n");
                }
            }
        }

        // Check for app=mac-rtl8139
        if (param_has_value(app_param, "mac-rtl8139")) {
            if (rtl8139_device_count >= MAX_NET_DEVICES) {
                puts("rtl8139: Maximum number of devices reached\n");
            } else {
                const driver_t *driver = rtl8139_get_driver();
                if (!driver) {
                    puts("rtl8139: Failed to get driver\n");
                    app_param = find_next_param(app_param, "app");
                    continue;
                }

                resource_t *resource = resource_acquire_available(driver, &rtl8139_contexts[rtl8139_device_count]);

                if (resource != NULL) {
                    resource_print_tag(resource);
                    puts(" Initializing...\n");

                    resource_print_tag(resource);
                    puts(" MAC: ");

                    uint8_t mac[6];
                    int result = rtl8139_get_mac(&rtl8139_contexts[rtl8139_device_count], mac);
                    if (result == 0) {
                        for (int i = 0; i < 6; i++) {
                            put_hex8(mac[i]);
                            if (i < 5) {
                                puts(":");
                            }
                        }
                    } else {
                        puts("(unavailable)");
                    }
                    puts("\n");

                    rtl8139_device_count++;
                } else {
                    puts("rtl8139: No available device\n");
                }
            }
        }

        // Find next app= parameter
        app_param = find_next_param(app_param, "app");
    }
}

// Helper function to find a parameter in the command line
static const char* find_param(const char* cmdline, const char* param) {
    if (cmdline == NULL || param == NULL) {
        return NULL;
    }

    size_t param_len = strlen(param);
    const char* pos = cmdline;

    while (*pos != '\0') {
        // Skip whitespace
        while (*pos == ' ' || *pos == '\t') {
            pos++;
        }

        // Check if this matches our parameter
        if (strncmp(pos, param, param_len) == 0) {
            // Ensure it's followed by '=' or end of word
            if (pos[param_len] == '=' || pos[param_len] == '\0' ||
                pos[param_len] == ' ' || pos[param_len] == '\t') {
                return pos;
            }
        }

        // Move to next space or end
        while (*pos != '\0' && *pos != ' ' && *pos != '\t') {
            pos++;
        }
    }

    return NULL;
}

// Helper function to find next occurrence of a parameter after given position
static const char* find_next_param(const char* start, const char* param) {
    if (start == NULL || param == NULL) {
        return NULL;
    }

    // Move past the current parameter
    while (*start != '\0' && *start != ' ' && *start != '\t') {
        start++;
    }

    // Find next occurrence
    return find_param(start, param);
}

// Helper function to check if a parameter has a specific value
static int param_has_value(const char* param_pos, const char* value) {
    if (param_pos == NULL || value == NULL) {
        return 0;
    }

    // Find the '=' sign
    while (*param_pos != '=' && *param_pos != '\0' &&
           *param_pos != ' ' && *param_pos != '\t') {
        param_pos++;
    }

    if (*param_pos != '=') {
        return 0;
    }

    param_pos++; // Skip '='

    size_t value_len = strlen(value);
    if (strncmp(param_pos, value, value_len) == 0) {
        // Check if value ends properly
        char next_char = param_pos[value_len];
        if (next_char == '\0' || next_char == ' ' || next_char == '\t') {
            return 1;
        }
    }

    return 0;
}
