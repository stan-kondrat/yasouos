#include "virtio_rng.h"

// Global device instance (simplified for single device)
static virtio_rng_device_t global_device;

// Device ID table for matching
static const device_id_t virtio_rng_id_table[] = {
    { "virtio,rng", VIRTIO_VENDOR_ID, VIRTIO_RNG_DEVICE_ID, "VirtIO-RNG (Legacy)" },
    { 0, 0, 0, 0 } // Terminator
};

// Lifecycle hooks
static int virtio_rng_init(void) {
    return 0;
}

static void virtio_rng_exit(void) {
}

// Driver operations
static void virtio_rng_remove([[maybe_unused]] const device_t *device) {
    global_device.initialized = 0;
}

static const driver_ops_t virtio_rng_ops = {
    .probe = virtio_rng_probe,
    .remove = virtio_rng_remove,
    .name = "virtio-rng"
};

// Driver descriptor
static const device_driver_t virtio_rng_driver = {
    .name = "virtio-rng",
    .version = "0.1.0",
    .type = DRIVER_TYPE_RANDOM,
    .id_table = virtio_rng_id_table,
    .ops = &virtio_rng_ops,
    .init = virtio_rng_init,
    .exit = virtio_rng_exit
};

driver_reg_result_t virtio_rng_register_driver(void) {
    return driver_register(&virtio_rng_driver);
}

int virtio_rng_probe(const device_t *device) {
    if (!device) {
        return -1;
    }

    global_device.device = *device;
    global_device.io_base = device->reg_base;
    global_device.initialized = 1;

    return 0;
}
