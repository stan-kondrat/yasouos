#include "virtio_blk.h"

// Device ID table for matching
static const device_id_t virtio_blk_id_table[] = {
    { "virtio,block", VIRTIO_VENDOR_ID, VIRTIO_BLK_DEVICE_ID, "VirtIO-Blk (Legacy)" },
    { 0, 0, 0, 0 } // Terminator
};

// Lifecycle hooks
static int virtio_blk_init_context(void *ctx, device_t *device) {
    // TODO: Initialize VirtIO block device context
    (void)ctx;
    (void)device;
    return 0;
}

static void virtio_blk_deinit_context(void *ctx) {
    // TODO: Deinitialize VirtIO block device context
    (void)ctx;
}

// Driver descriptor
static const driver_t virtio_blk_driver = {
    .name = "virtio-blk",
    .version = "0.1.0",
    .type = DRIVER_TYPE_STORAGE,
    .id_table = virtio_blk_id_table,
    .init_context = virtio_blk_init_context,
    .deinit_context = virtio_blk_deinit_context
};

const driver_t* virtio_blk_get_driver(void) {
    return &virtio_blk_driver;
}
