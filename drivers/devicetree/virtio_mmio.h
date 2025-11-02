#pragma once

#include "../../include/types.h"
#include "devicetree.h"

// VirtIO MMIO registers
#define VIRTIO_MMIO_MAGIC_VALUE      0x000
#define VIRTIO_MMIO_VERSION          0x004
#define VIRTIO_MMIO_DEVICE_ID        0x008
#define VIRTIO_MMIO_VENDOR_ID        0x00c

// VirtIO device types (not PCI IDs!)
#define VIRTIO_TYPE_NET     1
#define VIRTIO_TYPE_BLOCK   2
#define VIRTIO_TYPE_RNG     4

// Platform-specific configuration
typedef struct {
    uint64_t base_addr;
    uint64_t device_size;
    int device_count;
} virtio_mmio_config_t;

/**
 * Map VirtIO device type to PCI-style device ID for driver matching
 */
static inline uint16_t virtio_type_to_device_id(uint32_t device_type) {
    switch (device_type) {
        case VIRTIO_TYPE_NET:   return 0x1000;  // Legacy net
        case VIRTIO_TYPE_BLOCK: return 0x1001;  // Block
        case VIRTIO_TYPE_RNG:   return 0x1005;  // RNG
        default: return 0;
    }
}

/**
 * Safe MMIO read with address validation
 */
static inline int mmio_read32_safe(uint64_t addr, uint32_t *value) {
    if (!value) {
        return -1;
    }

    // Basic alignment check
    if (addr & 0x3) {
        return -1;
    }

    *value = *(volatile uint32_t *)addr;
    return 0;
}

/**
 * Enumerate VirtIO MMIO devices
 */
int virtio_mmio_enumerate(const virtio_mmio_config_t *config,
                          dt_device_callback_t callback,
                          void *context);
