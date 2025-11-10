#include "virtio_rng.h"
#include "../../kernel/devices/virtio_mmio.h"
#include "../../common/common.h"

// VirtIO Vendor and Device IDs
#define VIRTIO_VENDOR_ID           0x1AF4
#define VIRTIO_RNG_DEVICE_ID       0x1005

// Descriptor flags
#define VRING_DESC_F_WRITE 2

// Device ID table for matching
static const device_id_t virtio_rng_id_table[] = {
    { "virtio,rng", VIRTIO_VENDOR_ID, VIRTIO_RNG_DEVICE_ID, "VirtIO-RNG (Legacy)" },
    { 0, 0, 0, 0 } // Terminator
};

// Forward declaration
static int virtio_rng_init_context(void *ctx, device_t *device);
static void virtio_rng_deinit_context(void *ctx);

// Driver descriptor
static const driver_t virtio_rng_driver = {
    .name = "virtio-rng",
    .version = "0.1.0",
    .type = DRIVER_TYPE_RANDOM,
    .id_table = virtio_rng_id_table,
    .init_context = virtio_rng_init_context,
    .deinit_context = virtio_rng_deinit_context
};

const driver_t* virtio_rng_get_driver(void) {
    return &virtio_rng_driver;
}

static int virtio_rng_init_context(void *ctx, device_t *device) {
    if (!ctx || !device) {
        return -1;
    }

    virtio_rng_t *rng_ctx = (virtio_rng_t *)ctx;
    rng_ctx->io_base = device->reg_base;
    uint64_t base = rng_ctx->io_base;

    // Reset device
    mmio_write32(base + VIRTIO_MMIO_STATUS, 0);

    // Set ACKNOWLEDGE status bit
    mmio_write32(base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);

    // Set DRIVER status bit
    mmio_write32(base + VIRTIO_MMIO_STATUS,
                 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    // Read and accept device features (we don't need any specific features for RNG)
    (void)mmio_read32(base + VIRTIO_MMIO_DEVICE_FEATURES);
    mmio_write32(base + VIRTIO_MMIO_DRIVER_FEATURES, 0);  // No features needed

    // Set FEATURES_OK
    mmio_write32(base + VIRTIO_MMIO_STATUS,
                 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

    // Initialize virtqueue (queue 0 for RNG)
    mmio_write32(base + VIRTIO_MMIO_QUEUE_SEL, 0);

    // Check queue size
    uint32_t queue_size = mmio_read32(base + VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (queue_size == 0 || queue_size < VIRTIO_RNG_QUEUE_SIZE) {
        mmio_write32(base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        return -1;
    }

    // Set queue size
    mmio_write32(base + VIRTIO_MMIO_QUEUE_NUM, VIRTIO_RNG_QUEUE_SIZE);

    // Set queue alignment (legacy: 4096 bytes)
    mmio_write32(base + VIRTIO_MMIO_QUEUE_ALIGN, 4096);

    // Initialize queue memory
    memset(&rng_ctx->queue, 0, sizeof(virtio_rng_queue_t));
    rng_ctx->queue.last_used_idx = 0;

    // Set queue PFN (physical page number)
    uint64_t queue_addr = (uint64_t)&rng_ctx->queue;
    uint32_t queue_pfn = queue_addr >> 12;  // Divide by 4096
    mmio_write32(base + VIRTIO_MMIO_QUEUE_PFN, queue_pfn);

    // Set DRIVER_OK status bit
    mmio_write32(base + VIRTIO_MMIO_STATUS,
                 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                 VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

    rng_ctx->initialized = true;

    puts("virtio-rng: device initialized\n");
    return 0;
}

static void virtio_rng_deinit_context(void *ctx) {
    if (!ctx) {
        return;
    }

    virtio_rng_t *rng_ctx = (virtio_rng_t *)ctx;

    if (!rng_ctx->initialized) {
        return;
    }

    // Reset device
    uint64_t base = rng_ctx->io_base;
    mmio_write32(base + VIRTIO_MMIO_STATUS, 0);

    rng_ctx->initialized = false;

    puts("virtio-rng: device deinitialized\n");
}

int virtio_rng_read(virtio_rng_t *ctx, uint8_t *buffer, size_t size) {
    if (!ctx || !buffer || size == 0) {
        return -1;
    }

    if (!ctx->initialized) {
        return -1;
    }

    uint64_t base = ctx->io_base;

    // Limit read size to our internal buffer
    if (size > sizeof(ctx->buffer)) {
        size = sizeof(ctx->buffer);
    }

    // Setup descriptor for the buffer (device will write to it)
    uint16_t desc_idx = 0;
    ctx->queue.desc[desc_idx].addr = (uint64_t)ctx->buffer;
    ctx->queue.desc[desc_idx].len = size;
    ctx->queue.desc[desc_idx].flags = VRING_DESC_F_WRITE;  // Device writes
    ctx->queue.desc[desc_idx].next = 0;

    // Add descriptor to available ring
    uint16_t avail_idx = ctx->queue.avail.idx % VIRTIO_RNG_QUEUE_SIZE;
    ctx->queue.avail.ring[avail_idx] = desc_idx;
    ctx->queue.avail.idx++;

    // Notify device (kick)
    mmio_write32(base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);

    // Poll for completion (simple busy wait)
    uint16_t last_used = ctx->queue.last_used_idx;
    int timeout = 100000;
    while (ctx->queue.used.idx == last_used && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        puts("virtio-rng: timeout waiting for data\n");
        return -1;
    }

    // Get the used descriptor
    uint16_t used_idx = last_used % VIRTIO_RNG_QUEUE_SIZE;
    virtio_rng_used_elem_t *used_elem = &ctx->queue.used.ring[used_idx];
    uint32_t bytes_read = used_elem->len;

    // Update last used index
    ctx->queue.last_used_idx++;

    // Copy data to user buffer
    if (bytes_read > size) {
        bytes_read = size;
    }

    memcpy(buffer, ctx->buffer, bytes_read);

    return (int)bytes_read;
}
