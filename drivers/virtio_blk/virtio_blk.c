#include "virtio_blk.h"

// Global device instance (simplified for single device)
static virtio_blk_device_t global_device;

// Device ID table for matching
static const device_id_t virtio_blk_id_table[] = {
    { "virtio,block", VIRTIO_VENDOR_ID, VIRTIO_BLK_DEVICE_ID, "VirtIO-Blk (Legacy)" },
    { 0, 0, 0, 0 } // Terminator
};

// Lifecycle hooks
static int virtio_blk_init(void) {
    return 0;
}

static void virtio_blk_exit(void) {
}

// Driver operations
static void virtio_blk_remove([[maybe_unused]] const dt_device_t *device) {
    global_device.initialized = 0;
}

static const driver_ops_t virtio_blk_ops = {
    .probe = virtio_blk_probe,
    .remove = virtio_blk_remove,
    .name = "virtio-blk"
};

// Driver descriptor
static const device_driver_t virtio_blk_driver = {
    .name = "virtio-blk",
    .version = "0.1.0",
    .type = DRIVER_TYPE_STORAGE,
    .id_table = virtio_blk_id_table,
    .ops = &virtio_blk_ops,
    .init = virtio_blk_init,
    .exit = virtio_blk_exit
};

driver_reg_result_t virtio_blk_register_driver(void) {
    return driver_register(&virtio_blk_driver);
}

int virtio_blk_probe(const dt_device_t *device) {
    if (!device) {
        return -1;
    }

    global_device.dt_dev = *device;
    global_device.io_base = device->reg_base;
    global_device.initialized = 1;

    return 0;
}
