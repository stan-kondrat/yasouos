#include "netdev_mac.h"
#include "../../common/common.h"

#define MAX_NET_DEVICES 4
static e1000_t contexts[MAX_NET_DEVICES] = {0};
static int device_count = 0;

void app_mac_e1000(void) {
    if (device_count >= MAX_NET_DEVICES) {
        puts("e1000: Maximum number of devices reached\n");
        return;
    }

    const driver_t *driver = e1000_get_driver();
    if (!driver) {
        puts("e1000: Failed to get driver\n");
        return;
    }

    resource_t *resource = resource_acquire_available(driver, &contexts[device_count]);

    if (resource != NULL) {
        resource_print_tag(resource);
        puts(" Initializing...\n");

        resource_print_tag(resource);
        puts(" MAC: ");

        uint8_t mac[6];
        int result = e1000_get_mac(&contexts[device_count], mac);
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
        puts("e1000: No available device\n");
    }
}
