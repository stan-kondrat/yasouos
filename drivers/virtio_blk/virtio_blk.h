#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"

// VirtIO Vendor and Device IDs
#define VIRTIO_VENDOR_ID           0x1AF4
#define VIRTIO_BLK_DEVICE_ID       0x1001

// VirtIO Block Device Structure
typedef struct {
    device_t device;
    uint64_t io_base;
    int initialized;
} virtio_blk_device_t;

/**
 * Get virtio-blk driver descriptor
 * @return Pointer to driver descriptor
 */
const driver_t* virtio_blk_get_driver(void);
