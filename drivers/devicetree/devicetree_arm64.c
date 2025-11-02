#include "devicetree.h"
#include "virtio_mmio.h"
#include "../../include/common.h"

// ARM64 QEMU virt machine - VirtIO MMIO devices
// QEMU places up to 32 virtio-mmio devices at these addresses
static const virtio_mmio_config_t arm64_config = {
    .base_addr = 0x0a000000,
    .device_size = 0x200,
    .device_count = 32
};

int dt_init(void) {
    return 0;
}

int dt_enumerate_devices(dt_device_callback_t callback, void *context) {
    return virtio_mmio_enumerate(&arm64_config, callback, context);
}

int dt_find_device([[maybe_unused]] const char *compatible,
                   [[maybe_unused]] dt_device_t *device) {
    return 0;
}

const char *dt_get_device_name([[maybe_unused]] uint16_t vendor_id,
                               [[maybe_unused]] uint16_t device_id) {
    return NULL;
}
