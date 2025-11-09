#include "virtio_mmio.h"
#include "../../common/common.h"

int virtio_mmio_probe_device(device_t *device) {
    if (!device || device->reg_base == 0) {
        return -1;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t device_type = 0;

    // Read magic value to check if device exists
    if (mmio_read32_safe(device->reg_base + VIRTIO_MMIO_MAGIC_VALUE, &magic) != 0) {
        return -1;
    }

    // VirtIO magic is 0x74726976 ("virt" in little endian)
    if (magic != 0x74726976) {
        return -1;  // No device here
    }

    // Read version (should be 1 or 2)
    if (mmio_read32_safe(device->reg_base + VIRTIO_MMIO_VERSION, &version) != 0) {
        return -1;
    }

    if (version == 0 || version > 2) {
        return -1;  // Invalid version
    }

    // Read device type (VirtIO device type, not PCI device ID)
    if (mmio_read32_safe(device->reg_base + VIRTIO_MMIO_DEVICE_ID, &device_type) != 0) {
        return -1;
    }

    // Skip if device type is 0 (no device)
    if (device_type == 0) {
        return -1;
    }

    // Convert VirtIO device type to PCI-style device ID for driver matching
    uint16_t device_id = virtio_type_to_device_id(device_type);
    if (device_id == 0) {
        return -1;  // Unknown device type
    }

    // Populate device structure
    device->vendor_id = 0x1AF4;  // Standard VirtIO vendor ID
    device->device_id = device_id;

    return 0;
}

int virtio_mmio_enumerate(const virtio_mmio_config_t *config,
                          device_callback_t callback,
                          void *context) {
    if (!config) {
        return 0;
    }

    int device_count = 0;

    for (int i = 0; i < config->device_count; i++) {
        uint64_t base = config->base_addr + (i * config->device_size);
        uint32_t magic = 0;
        uint32_t version = 0;
        uint32_t device_type = 0;

        // Read magic value to check if device exists
        if (mmio_read32_safe(base + VIRTIO_MMIO_MAGIC_VALUE, &magic) != 0) {
            continue;
        }

        // VirtIO magic is 0x74726976 ("virt" in little endian)
        if (magic != 0x74726976) {
            continue;  // No device here
        }

        // Read version (should be 1 or 2)
        if (mmio_read32_safe(base + VIRTIO_MMIO_VERSION, &version) != 0) {
            continue;
        }

        if (version == 0 || version > 2) {
            continue;  // Invalid version
        }

        // Read device type (VirtIO device type, not PCI device ID)
        if (mmio_read32_safe(base + VIRTIO_MMIO_DEVICE_ID, &device_type) != 0) {
            continue;
        }

        // Skip if device type is 0 (no device)
        if (device_type == 0) {
            continue;
        }

        // Convert VirtIO device type to PCI-style device ID for driver matching
        uint16_t device_id = virtio_type_to_device_id(device_type);
        if (device_id == 0) {
            continue;  // Unknown device type
        }

        // Create device info
        device_t dev_info = {
            .compatible = "virtio,mmio",
            .name = NULL,
            .reg_base = base,
            .reg_size = config->device_size,
            .vendor_id = 0x1AF4,  // Standard VirtIO vendor ID for matching
            .device_id = device_id,
            .bus = 0,
            .device_num = i,
            .function = 0
        };

        if (callback) {
            callback(&dev_info, context);
        }

        device_count++;
    }

    return device_count;
}
