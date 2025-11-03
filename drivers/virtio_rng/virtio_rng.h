#pragma once

#include "../../common/types.h"
#include "../../kernel/drivers/drivers.h"
#include "../../kernel/devices/devices.h"

// VirtIO Vendor and Device IDs
#define VIRTIO_VENDOR_ID           0x1AF4
#define VIRTIO_RNG_DEVICE_ID       0x1005

// VirtIO RNG Device Structure
typedef struct {
    device_t device;
    uint64_t io_base;
    int initialized;
} virtio_rng_device_t;

/**
 * Register virtio-rng driver with the system
 * @return driver_reg_result_t status code
 */
driver_reg_result_t virtio_rng_register_driver(void);

/**
 * Probe function called by driver registry
 *
 * @param device Device tree device to probe
 * @return 0 on success, -1 on failure
 */
int virtio_rng_probe(const device_t *device);
