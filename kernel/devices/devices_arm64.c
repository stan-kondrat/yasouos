#include "devices.h"
#include "virtio_mmio.h"
#include "../../common/common.h"

// ARM64 QEMU virt machine - VirtIO MMIO devices
// QEMU places up to 32 virtio-mmio devices at these addresses
static const virtio_mmio_config_t arm64_config = {
    .base_addr = 0x0a000000,
    .device_size = 0x200,
    .device_count = 32
};

int devices_enumerate(device_callback_t callback, void *context) {
    return virtio_mmio_enumerate(&arm64_config, callback, context);
}

int devices_find([[maybe_unused]] const char *compatible,
                   [[maybe_unused]] device_t *device) {
    return 0;
}

const char *devices_get_name([[maybe_unused]] uint16_t vendor_id,
                               [[maybe_unused]] uint16_t device_id) {
    return NULL;
}
