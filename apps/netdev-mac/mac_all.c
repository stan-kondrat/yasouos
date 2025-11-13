#include "netdev_mac.h"
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
        int result = -1;

        // Determine driver type and get MAC
        if (devices[i].driver == rtl8139_get_driver()) {
            result = rtl8139_get_mac((rtl8139_t *)devices[i].context, mac);
        } else if (devices[i].driver == virtio_net_get_driver()) {
            result = virtio_net_get_mac((virtio_net_t *)devices[i].context, mac);
        } else if (devices[i].driver == e1000_get_driver()) {
            result = e1000_get_mac((e1000_t *)devices[i].context, mac);
        }

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
