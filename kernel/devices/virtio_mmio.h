#pragma once

#include "../../common/types.h"
#include "devices.h"

// VirtIO MMIO registers (Legacy/v1)
#define VIRTIO_MMIO_MAGIC_VALUE           0x000
#define VIRTIO_MMIO_VERSION               0x004
#define VIRTIO_MMIO_DEVICE_ID             0x008
#define VIRTIO_MMIO_VENDOR_ID             0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES       0x010
#define VIRTIO_MMIO_DRIVER_FEATURES       0x020
#define VIRTIO_MMIO_QUEUE_SEL             0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX         0x034
#define VIRTIO_MMIO_QUEUE_NUM             0x038
#define VIRTIO_MMIO_QUEUE_ALIGN           0x03c
#define VIRTIO_MMIO_QUEUE_PFN             0x040
#define VIRTIO_MMIO_QUEUE_NOTIFY          0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS      0x060
#define VIRTIO_MMIO_INTERRUPT_ACK         0x064
#define VIRTIO_MMIO_STATUS                0x070

// VirtIO status bits
#define VIRTIO_STATUS_ACKNOWLEDGE         1
#define VIRTIO_STATUS_DRIVER              2
#define VIRTIO_STATUS_DRIVER_OK           4
#define VIRTIO_STATUS_FEATURES_OK         8
#define VIRTIO_STATUS_FAILED              128

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

    *value = *(volatile uint32_t *)(uintptr_t)addr;
    return 0;
}

/**
 * Safe MMIO write with address validation
 */
static inline void mmio_write32(uint64_t addr, uint32_t value) {
    *(volatile uint32_t *)(uintptr_t)addr = value;
}

/**
 * MMIO read32 (no error checking for performance)
 */
static inline uint32_t mmio_read32(uint64_t addr) {
    return *(volatile uint32_t *)(uintptr_t)addr;
}

/**
 * Probe a single VirtIO MMIO device and populate vendor/device IDs
 * @param device Device structure to populate
 * @return 0 if valid VirtIO device, -1 otherwise
 */
int virtio_mmio_probe_device(device_t *device);

/**
 * Enumerate VirtIO MMIO devices
 */
int virtio_mmio_enumerate(const virtio_mmio_config_t *config,
                          device_callback_t callback,
                          void *context);
