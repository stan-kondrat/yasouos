#include "virtio_net.h"

// Global device instance (simplified for single device)
static virtio_net_device_t global_device;

// Device ID table for matching
static const device_id_t virtio_net_id_table[] = {
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_MODERN, "VirtIO-Net (1.0+)" },
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_LEGACY, "VirtIO-Net (Legacy)" },
    { 0, 0, 0, 0 } // Terminator
};

// Lifecycle hooks (for future dynamic loading)
static int virtio_net_init(void) {
    // Called once when driver is registered
    // Future: Initialize driver-wide resources
    return 0;
}

static void virtio_net_exit(void) {
    // Called when driver is unregistered
    // Future: Clean up driver-wide resources
}

// Driver operations
static void virtio_net_remove([[maybe_unused]] const device_t *device) {
    global_device.initialized = 0;
}

static const driver_ops_t virtio_net_ops = {
    .probe = virtio_net_probe,
    .remove = virtio_net_remove,
    .name = "virtio-net"
};

// Driver descriptor
static const device_driver_t virtio_net_driver = {
    .name = "virtio-net",
    .version = "0.1.0",
    .type = DRIVER_TYPE_NETWORK,
    .id_table = virtio_net_id_table,
    .ops = &virtio_net_ops,
    .init = virtio_net_init,
    .exit = virtio_net_exit
};

driver_reg_result_t virtio_net_register_driver(void) {
    return driver_register(&virtio_net_driver);
}

int virtio_net_probe(const device_t *device) {
    if (!device) {
        return -1;
    }

    global_device.device = *device;
    global_device.io_base = device->reg_base;
    global_device.initialized = 1;

    return 0;
}

uint64_t virtio_net_get_io_base(const virtio_net_device_t *device) {
    if (!device || !device->initialized) {
        return 0;
    }

    return device->io_base;
}
