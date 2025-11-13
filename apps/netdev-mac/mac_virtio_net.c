#include "netdev_mac.h"
#include "../../common/common.h"

#define MAX_NET_DEVICES 4
static virtio_net_t contexts[MAX_NET_DEVICES] = {0};
static int device_count = 0;

void app_mac_virtio_net(void) {
    if (device_count >= MAX_NET_DEVICES) {
        puts("virtio-net: Maximum number of devices reached\n");
        return;
    }

    const driver_t *driver = virtio_net_get_driver();
    resource_t *resource = resource_acquire_available(driver, &contexts[device_count]);

    if (resource != NULL) {
        resource_print_tag(resource);
        puts(" Initializing...\n");

        resource_print_tag(resource);
        puts(" MAC: ");

        uint8_t mac[6];
        int result = virtio_net_get_mac(&contexts[device_count], mac);
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
        puts("virtio-net: No available device\n");
    }
}
