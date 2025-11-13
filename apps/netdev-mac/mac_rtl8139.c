#include "netdev_mac.h"
#include "../../common/common.h"

#define MAX_NET_DEVICES 4
static rtl8139_t contexts[MAX_NET_DEVICES] = {0};
static int device_count = 0;

void app_mac_rtl8139(void) {
    if (device_count >= MAX_NET_DEVICES) {
        puts("rtl8139: Maximum number of devices reached\n");
        return;
    }

    const driver_t *driver = rtl8139_get_driver();
    if (!driver) {
        puts("rtl8139: Failed to get driver\n");
        return;
    }

    resource_t *resource = resource_acquire_available(driver, &contexts[device_count]);

    if (resource != NULL) {
        resource_print_tag(resource);
        puts(" Initializing...\n");

        resource_print_tag(resource);
        puts(" MAC: ");

        uint8_t mac[6];
        int result = rtl8139_get_mac(&contexts[device_count], mac);
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

        device_count++;
    } else {
        puts("rtl8139: No available device\n");
    }
}
