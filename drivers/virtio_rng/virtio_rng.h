#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"
#include "../../kernel/resources/resources.h"

// VirtIO RNG queue size
#define VIRTIO_RNG_QUEUE_SIZE 8

// Virtqueue descriptor
typedef struct {
    volatile uint64_t addr;
    volatile uint32_t len;
    volatile uint16_t flags;
    volatile uint16_t next;
} virtio_rng_desc_t;

// Virtqueue available ring
typedef struct {
    volatile uint16_t flags;
    volatile uint16_t idx;
    volatile uint16_t ring[VIRTIO_RNG_QUEUE_SIZE];
    volatile uint16_t used_event;  // Only if VIRTIO_F_EVENT_IDX
} virtio_rng_avail_t;

// Virtqueue used ring element
typedef struct {
    volatile uint32_t id;
    volatile uint32_t len;
} virtio_rng_used_elem_t;

// Virtqueue used ring
typedef struct {
    volatile uint16_t flags;
    volatile uint16_t idx;
    virtio_rng_used_elem_t ring[VIRTIO_RNG_QUEUE_SIZE];
    volatile uint16_t avail_event;  // Only if VIRTIO_F_EVENT_IDX
} virtio_rng_used_t;

// Virtqueue structure (VirtIO legacy MMIO split queue layout)
// Layout per spec:
//   - Descriptor table: 16 * queue_size (128 bytes for size=8)
//   - Available ring: 6 + 2 * queue_size (20 bytes for size=8, without event_idx)
//   - Padding to next 4096-byte boundary
//   - Used ring: 6 + 8 * queue_size (70 bytes for size=8, without event_idx)
// Note: event_idx fields (used_event, avail_event) only used if VIRTIO_F_EVENT_IDX negotiated
typedef struct {
    virtio_rng_desc_t desc[VIRTIO_RNG_QUEUE_SIZE];      // 0-127 (128 bytes)
    virtio_rng_avail_t avail;                            // 128-151 (24 bytes with event)
    uint8_t padding[4096 - 128 - 24];                    // Pad to 4096 (used ring must be page-aligned)
    virtio_rng_used_t used;                              // 4096+ (72 bytes with event)
    uint16_t last_used_idx;                              // Driver's tracking variable (not part of virtqueue)
} __attribute__((aligned(4096))) virtio_rng_queue_t;

// VirtIO transport types
typedef enum {
    VIRTIO_TRANSPORT_MMIO = 0,  // Memory-mapped I/O (ARM64, RISC-V)
    VIRTIO_TRANSPORT_PCI = 1     // PCI I/O ports (AMD64)
} virtio_transport_t;

/**
 * VirtIO RNG context (user-allocated)
 * Allocate on stack: virtio_rng_t rng = {0};
 */
typedef struct {
    uint64_t io_base;
    bool initialized;
    virtio_transport_t transport;  // Transport type (MMIO or PCI)
    virtio_rng_queue_t queue;
    uint8_t buffer[64];
    bool desc_in_use[VIRTIO_RNG_QUEUE_SIZE];  // Track which descriptors are in use
} virtio_rng_t;

/**
 * Get virtio-rng driver descriptor
 * @return Pointer to driver descriptor
 */
const driver_t* virtio_rng_get_driver(void);

/**
 * Read random bytes from hardware RNG
 *
 * @param ctx Initialized context
 * @param buffer Buffer to fill with random data
 * @param size Number of bytes to read
 * @return Number of bytes read, or -1 on error
 */
int virtio_rng_read(virtio_rng_t *ctx, uint8_t *buffer, size_t size);
