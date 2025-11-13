#pragma once

#include "../../common/types.h"
#include "../../drivers/virtio_net/virtio_net.h"
#include "../../drivers/e1000/e1000.h"
#include "../../drivers/rtl8139/rtl8139.h"
#include "../../kernel/resources/resources.h"

typedef struct {
    resource_t *resource;
    const driver_t *driver;
    void *context;
} device_entry_t;

int netdev_acquire_all(device_entry_t *devices, int max_devices);
int netdev_get_mac(const device_entry_t *device, uint8_t mac[6]);
int netdev_transmit(const device_entry_t *device, const uint8_t *packet, size_t length);
int netdev_receive(const device_entry_t *device, uint8_t *buffer, size_t buffer_size, size_t *received_length);
