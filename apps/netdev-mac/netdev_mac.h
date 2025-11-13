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

void app_mac_virtio_net(void);
void app_mac_e1000(void);
void app_mac_rtl8139(void);
void app_mac_all(void);
