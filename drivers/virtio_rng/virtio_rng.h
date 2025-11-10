#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"
#include "../../kernel/resources/resources.h"

// VirtIO RNG queue size
#define VIRTIO_RNG_QUEUE_SIZE 8

// Virtqueue descriptor
typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} virtio_rng_desc_t;

// Virtqueue available ring
typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_RNG_QUEUE_SIZE];
} virtio_rng_avail_t;

// Virtqueue used ring element
typedef struct {
    uint32_t id;
    uint32_t len;
} virtio_rng_used_elem_t;

// Virtqueue used ring
typedef struct {
    uint16_t flags;
    uint16_t idx;
    virtio_rng_used_elem_t ring[VIRTIO_RNG_QUEUE_SIZE];
} virtio_rng_used_t;

// Virtqueue structure
typedef struct {
    virtio_rng_desc_t desc[VIRTIO_RNG_QUEUE_SIZE];
    virtio_rng_avail_t avail;
    uint8_t padding[4096];
    virtio_rng_used_t used;
    uint16_t last_used_idx;
} virtio_rng_queue_t;

/**
 * VirtIO RNG context (user-allocated)
 * Allocate on stack: virtio_rng_t rng = {0};
 */
typedef struct {
    uint64_t io_base;
    bool initialized;
    virtio_rng_queue_t queue;
    uint8_t buffer[64];
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
