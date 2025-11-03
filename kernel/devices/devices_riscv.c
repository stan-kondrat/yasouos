#include "devices.h"
#include "virtio_mmio.h"
#include "../../common/common.h"

// RISC-V QEMU virt machine - VirtIO MMIO devices
// QEMU places up to 8 virtio-mmio devices at these addresses on RISC-V
static const virtio_mmio_config_t riscv_config = {
    .base_addr = 0x10001000,
    .device_size = 0x1000,
    .device_count = 8
};

int devices_enumerate(device_callback_t callback, void *context) {
    return virtio_mmio_enumerate(&riscv_config, callback, context);
}

int devices_find([[maybe_unused]] const char *compatible,
                   [[maybe_unused]] device_t *device) {
    return 0;
}

const char *devices_get_name([[maybe_unused]] uint16_t vendor_id,
                               [[maybe_unused]] uint16_t device_id) {
    return NULL;
}
