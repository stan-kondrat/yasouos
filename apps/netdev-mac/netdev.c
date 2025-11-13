#include "netdev.h"
#include "../../common/common.h"

int netdev_acquire_all(device_entry_t *devices, int max_devices) {
    int device_count = 0;

    while (device_count < max_devices) {
        resource_t *resource = NULL;
        const driver_t *driver = NULL;

        // Try RTL8139
        driver = rtl8139_get_driver();
        if (driver != NULL) {
            static rtl8139_t rtl8139_contexts[12] = {0};
            static int rtl8139_index = 0;
            if (rtl8139_index < 12) {
                resource = resource_acquire_available(driver, &rtl8139_contexts[rtl8139_index]);
                if (resource != NULL) {
                    devices[device_count].resource = resource;
                    devices[device_count].driver = driver;
                    devices[device_count].context = &rtl8139_contexts[rtl8139_index];
                    rtl8139_index++;
                    device_count++;
                    continue;
                }
            }
        }

        // Try VirtIO-Net
        driver = virtio_net_get_driver();
        if (driver != NULL) {
            static virtio_net_t virtio_net_contexts[12] = {0};
            static int virtio_net_index = 0;
            if (virtio_net_index < 12) {
                resource = resource_acquire_available(driver, &virtio_net_contexts[virtio_net_index]);
                if (resource != NULL) {
                    devices[device_count].resource = resource;
                    devices[device_count].driver = driver;
                    devices[device_count].context = &virtio_net_contexts[virtio_net_index];
                    virtio_net_index++;
                    device_count++;
                    continue;
                }
            }
        }

        // Try E1000
        driver = e1000_get_driver();
        if (driver != NULL) {
            static e1000_t e1000_contexts[12] = {0};
            static int e1000_index = 0;
            if (e1000_index < 12) {
                resource = resource_acquire_available(driver, &e1000_contexts[e1000_index]);
                if (resource != NULL) {
                    devices[device_count].resource = resource;
                    devices[device_count].driver = driver;
                    devices[device_count].context = &e1000_contexts[e1000_index];
                    e1000_index++;
                    device_count++;
                    continue;
                }
            }
        }

        // No more devices available
        break;
    }

    return device_count;
}

int netdev_get_mac(const device_entry_t *device, uint8_t mac[6]) {
    if (device == NULL || mac == NULL) {
        return -1;
    }

    if (device->driver == rtl8139_get_driver()) {
        return rtl8139_get_mac((rtl8139_t *)device->context, mac);
    } else if (device->driver == virtio_net_get_driver()) {
        return virtio_net_get_mac((virtio_net_t *)device->context, mac);
    } else if (device->driver == e1000_get_driver()) {
        return e1000_get_mac((e1000_t *)device->context, mac);
    }

    return -1;
}

int netdev_transmit(const device_entry_t *device, const uint8_t *packet, size_t length) {
    if (device == NULL || packet == NULL || length == 0) {
        return -1;
    }

    // Dispatch to appropriate driver
    if (device->driver == virtio_net_get_driver()) {
        return virtio_net_transmit((virtio_net_t *)device->context, packet, length);
    } else if (device->driver == e1000_get_driver()) {
        // TODO: Implement e1000_transmit
        return -1;
    } else if (device->driver == rtl8139_get_driver()) {
        // TODO: Implement rtl8139_transmit
        return -1;
    }

    return -1;
}

int netdev_receive(const device_entry_t *device, uint8_t *buffer, size_t buffer_size, size_t *received_length) {
    if (device == NULL || buffer == NULL || received_length == NULL) {
        return -1;
    }

    // Dispatch to appropriate driver
    if (device->driver == virtio_net_get_driver()) {
        return virtio_net_receive((virtio_net_t *)device->context, buffer, buffer_size, received_length);
    } else if (device->driver == e1000_get_driver()) {
        // TODO: Implement e1000_receive
        return -1;
    } else if (device->driver == rtl8139_get_driver()) {
        // TODO: Implement rtl8139_receive
        return -1;
    }

    return -1;
}
