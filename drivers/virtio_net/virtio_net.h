#pragma once

#include "../../include/types.h"
#include "../driver_registry.h"
#include "../devicetree/devicetree.h"

// VirtIO Vendor and Device IDs
#define VIRTIO_VENDOR_ID           0x1AF4
#define VIRTIO_NET_DEVICE_ID_LEGACY 0x1000
#define VIRTIO_NET_DEVICE_ID_MODERN 0x1041

// VirtIO Network Device
typedef struct {
    dt_device_t dt_dev;
    uint64_t io_base;
    int initialized;
} virtio_net_device_t;

/**
 * Register virtio-net driver with the system
 * @return driver_reg_result_t status code
 */
driver_reg_result_t virtio_net_register_driver(void);

/**
 * Probe function called by driver registry
 *
 * @param device Device tree device to probe
 * @return 0 on success, -1 on failure
 */
int virtio_net_probe(const dt_device_t *device);

/**
 * Get I/O base address for virtio-net device
 *
 * @param device VirtIO network device
 * @return I/O base address or 0 if not initialized
 */
uint64_t virtio_net_get_io_base(const virtio_net_device_t *device);
