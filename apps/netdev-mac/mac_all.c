#include "netdev.h"
#include "../../common/common.h"

#define MAX_DEVICES 12

void app_mac_all(void) {
    device_entry_t devices[MAX_DEVICES] = {0};
    int device_count = netdev_acquire_all(devices, MAX_DEVICES);

    // Print MAC addresses for all acquired devices
    for (int i = 0; i < device_count; i++) {
        resource_print_tag(devices[i].resource);
        puts(" Initializing...\n");

        resource_print_tag(devices[i].resource);
        puts(" MAC: ");

        uint8_t mac[6];
        int result = netdev_get_mac(&devices[i], mac);

        if (result == 0) {
            for (int j = 0; j < 6; j++) {
                put_hex8(mac[j]);
                if (j < 5) {
                    puts(":");
                }
            }
        } else {
            puts("(unavailable)");
        }
        puts("\n");
    }
}
