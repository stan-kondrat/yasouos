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

// Translate MMIO register offsets to PCI register offsets
static inline uint16_t virtio_translate_offset(virtio_transport_t transport, uint16_t mmio_offset) {
#if defined(__x86_64__) || defined(__i386__)
    if (transport == VIRTIO_TRANSPORT_PCI) {
        // Map MMIO offsets to PCI legacy I/O space offsets
        switch (mmio_offset) {
            case VIRTIO_MMIO_DEVICE_FEATURES: return VIRTIO_PCI_DEVICE_FEATURES;
            case VIRTIO_MMIO_DRIVER_FEATURES: return VIRTIO_PCI_DRIVER_FEATURES;
            case VIRTIO_MMIO_QUEUE_PFN:       return VIRTIO_PCI_QUEUE_PFN;
            case VIRTIO_MMIO_QUEUE_NUM:       return VIRTIO_PCI_QUEUE_NUM;
            case VIRTIO_MMIO_QUEUE_SEL:       return VIRTIO_PCI_QUEUE_SEL;
            case VIRTIO_MMIO_QUEUE_NOTIFY:    return VIRTIO_PCI_QUEUE_NOTIFY;
            case VIRTIO_MMIO_STATUS:          return VIRTIO_PCI_STATUS;
            case VIRTIO_MMIO_INTERRUPT_STATUS: return VIRTIO_PCI_ISR_STATUS;
            // PCI doesn't have these registers
            case VIRTIO_MMIO_GUEST_PAGE_SIZE:
            case VIRTIO_MMIO_QUEUE_NUM_MAX:
            case VIRTIO_MMIO_QUEUE_ALIGN:
                return 0xFFFF; // Invalid offset marker
            default:
                return mmio_offset;
        }
    }
#else
    (void)transport; // Unused on non-x86 architectures
#endif
    return mmio_offset;
}

// Transport-agnostic register access functions
static inline uint32_t virtio_read32(virtio_rng_t *ctx, uint16_t mmio_offset) {
    uint16_t offset = virtio_translate_offset(ctx->transport, mmio_offset);
    if (offset == 0xFFFF) {
        return 0; // Register doesn't exist in this transport
    }

#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_TRANSPORT_PCI) {
        return virtio_pci_read32((uint16_t)ctx->io_base, offset);
    }
#endif
    return mmio_read32(ctx->io_base + offset);
}

static inline uint16_t virtio_read16(virtio_rng_t *ctx, uint16_t mmio_offset) {
    uint16_t offset = virtio_translate_offset(ctx->transport, mmio_offset);
    if (offset == 0xFFFF) {
        return 0;
    }

#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_TRANSPORT_PCI) {
        return io_inw((uint16_t)ctx->io_base + offset);
    }
#endif
    return (uint16_t)mmio_read32(ctx->io_base + offset);
}

static inline uint8_t virtio_read8(virtio_rng_t *ctx, uint16_t mmio_offset) {
    uint16_t offset = virtio_translate_offset(ctx->transport, mmio_offset);
    if (offset == 0xFFFF) {
        return 0;
    }

#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_TRANSPORT_PCI) {
        return io_inb((uint16_t)ctx->io_base + offset);
    }
#endif
    return (uint8_t)mmio_read32(ctx->io_base + offset);
}

static inline void virtio_write32(virtio_rng_t *ctx, uint16_t mmio_offset, uint32_t value) {
    uint16_t offset = virtio_translate_offset(ctx->transport, mmio_offset);
    if (offset == 0xFFFF) {
        return; // Register doesn't exist in this transport
    }

#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_TRANSPORT_PCI) {
        virtio_pci_write32((uint16_t)ctx->io_base, offset, value);
        return;
    }
#endif
    mmio_write32(ctx->io_base + offset, value);
}

static inline void virtio_write16(virtio_rng_t *ctx, uint16_t mmio_offset, uint16_t value) {
    uint16_t offset = virtio_translate_offset(ctx->transport, mmio_offset);
    if (offset == 0xFFFF) {
        return;
    }

#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_TRANSPORT_PCI) {
        io_outw((uint16_t)ctx->io_base + offset, value);
        return;
    }
#endif
    mmio_write32(ctx->io_base + offset, value);
}

static inline void virtio_write8(virtio_rng_t *ctx, uint16_t mmio_offset, uint8_t value) {
    uint16_t offset = virtio_translate_offset(ctx->transport, mmio_offset);
    if (offset == 0xFFFF) {
        return;
    }

#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_TRANSPORT_PCI) {
        io_outb((uint16_t)ctx->io_base + offset, value);
        return;
    }
#endif
    mmio_write32(ctx->io_base + offset, value);
}

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

    // Detect transport type: PCI devices have low I/O port addresses (< 0x10000)
    // MMIO devices have high memory addresses
    if (rng_ctx->io_base < 0x10000) {
        rng_ctx->transport = VIRTIO_TRANSPORT_PCI;
    } else {
        rng_ctx->transport = VIRTIO_TRANSPORT_MMIO;
    }

    // Check VirtIO version (register layout differs between PCI and MMIO)
    uint32_t version = virtio_read32(rng_ctx, VIRTIO_MMIO_VERSION);

    // For VirtIO-PCI legacy, version register may not exist or return 0
    // We'll proceed with legacy initialization regardless
    if (rng_ctx->transport == VIRTIO_TRANSPORT_MMIO && version != 1 && version != 2) {
        return -1;
    }

    // For PCI, assume legacy (version 1) layout
    if (rng_ctx->transport == VIRTIO_TRANSPORT_PCI) {
        version = 1;
    }

    // Reset device
    virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS, 0);

    // Set ACKNOWLEDGE status bit
    virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);

    // Set DRIVER status bit
    virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS,
                  VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    // Read and accept device features (we don't need any specific features for RNG)
    (void)virtio_read32(rng_ctx, VIRTIO_MMIO_DEVICE_FEATURES);
    virtio_write32(rng_ctx, VIRTIO_MMIO_DRIVER_FEATURES, 0);  // No features needed

    // For PCI legacy, FEATURES_OK doesn't exist in the status register
    // STATUS register flow is simpler: ACKNOWLEDGE -> DRIVER -> DRIVER_OK
    if (rng_ctx->transport == VIRTIO_TRANSPORT_MMIO) {
        // Set FEATURES_OK
        virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS,
                      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

        // Verify device accepted features (required by spec)
        uint8_t status = virtio_read8(rng_ctx, VIRTIO_MMIO_STATUS);
        if (!(status & VIRTIO_STATUS_FEATURES_OK)) {
            virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
            return -1;
        }
    }

    // Set guest page size (MMIO legacy v1 only, PCI doesn't have this)
    if (version == 1 && rng_ctx->transport == VIRTIO_TRANSPORT_MMIO) {
        virtio_write32(rng_ctx, VIRTIO_MMIO_GUEST_PAGE_SIZE, 4096);
    }

    // Initialize virtqueue (queue 0 for RNG)
    virtio_write16(rng_ctx, VIRTIO_MMIO_QUEUE_SEL, 0);

    // Check queue size
    // PCI: Read QUEUE_NUM to get max size, MMIO: Read QUEUE_NUM_MAX
    uint32_t queue_size;
    if (rng_ctx->transport == VIRTIO_TRANSPORT_PCI) {
        queue_size = virtio_read16(rng_ctx, VIRTIO_MMIO_QUEUE_NUM);
    } else {
        queue_size = virtio_read32(rng_ctx, VIRTIO_MMIO_QUEUE_NUM_MAX);
    }

    if (queue_size == 0 || queue_size < VIRTIO_RNG_QUEUE_SIZE) {
        virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        return -1;
    }

    // Set queue size (PCI uses 16-bit, MMIO uses 32-bit)
    if (rng_ctx->transport == VIRTIO_TRANSPORT_PCI) {
        virtio_write16(rng_ctx, VIRTIO_MMIO_QUEUE_NUM, VIRTIO_RNG_QUEUE_SIZE);
    } else {
        virtio_write32(rng_ctx, VIRTIO_MMIO_QUEUE_NUM, VIRTIO_RNG_QUEUE_SIZE);
    }

    // Initialize queue memory
    memset(&rng_ctx->queue, 0, sizeof(virtio_rng_queue_t));
    rng_ctx->queue.last_used_idx = 0;

    // Get queue address
    uint64_t queue_addr = (uint64_t)&rng_ctx->queue;

    if (version == 1) {
        // Legacy interface: use QUEUE_PFN
        // MMIO also needs QUEUE_ALIGN, PCI doesn't
        if (rng_ctx->transport == VIRTIO_TRANSPORT_MMIO) {
            virtio_write32(rng_ctx, VIRTIO_MMIO_QUEUE_ALIGN, 4096);
        }

        uint32_t queue_pfn = queue_addr >> 12;
        virtio_write32(rng_ctx, VIRTIO_MMIO_QUEUE_PFN, queue_pfn);

        // Verify queue was configured
        uint32_t readback_pfn = virtio_read32(rng_ctx, VIRTIO_MMIO_QUEUE_PFN);
        if (readback_pfn != queue_pfn) {
            virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
            return -1;
        }
    } else {
        // Modern interface (version 2): use QUEUE_DESC_LOW/HIGH, AVAIL_LOW/HIGH, USED_LOW/HIGH
        virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        return -1;
    }

    // Set DRIVER_OK status bit
    if (rng_ctx->transport == VIRTIO_TRANSPORT_MMIO) {
        virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS,
                      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                      VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);
    } else {
        // PCI legacy doesn't have FEATURES_OK
        virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS,
                      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                      VIRTIO_STATUS_DRIVER_OK);
    }

    // Verify DRIVER_OK was set
    uint8_t status = virtio_read8(rng_ctx, VIRTIO_MMIO_STATUS);
    if (!(status & VIRTIO_STATUS_DRIVER_OK)) {
        return -1;
    }

    // Small delay to let device process DRIVER_OK
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ __volatile__("nop");
    }

    rng_ctx->initialized = true;
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
    virtio_write8(rng_ctx, VIRTIO_MMIO_STATUS, 0);

    rng_ctx->initialized = false;
}

int virtio_rng_read(virtio_rng_t *ctx, uint8_t *buffer, size_t size) {
    if (!ctx || !buffer || size == 0) {
        return -1;
    }

    if (!ctx->initialized) {
        return -1;
    }

    // Limit read size to our internal buffer
    if (size > sizeof(ctx->buffer)) {
        size = sizeof(ctx->buffer);
    }

    // Find an available descriptor
    uint16_t desc_idx = 0;
    bool found = false;
    for (uint16_t i = 0; i < VIRTIO_RNG_QUEUE_SIZE; i++) {
        if (!ctx->desc_in_use[i]) {
            desc_idx = i;
            found = true;
            break;
        }
    }

    if (!found) {
        return -1;
    }

    // Mark descriptor as in use
    ctx->desc_in_use[desc_idx] = true;

    // Setup descriptor for the buffer (device will write to it)
    ctx->queue.desc[desc_idx].addr = (uint64_t)ctx->buffer;
    ctx->queue.desc[desc_idx].len = size;
    ctx->queue.desc[desc_idx].flags = VRING_DESC_F_WRITE;  // Device writes
    ctx->queue.desc[desc_idx].next = 0;

    // Add descriptor to available ring
    uint16_t avail_idx = ctx->queue.avail.idx % VIRTIO_RNG_QUEUE_SIZE;
    ctx->queue.avail.ring[avail_idx] = desc_idx;

    // Memory barrier: ensure descriptor and ring entry are written before idx update
    __sync_synchronize();

    ctx->queue.avail.idx++;

    // Memory barrier: ensure idx is written before QUEUE_NOTIFY
    __sync_synchronize();

    // Notify device (kick)
    // PCI: QUEUE_NOTIFY is 16-bit and takes queue index, MMIO: 32-bit
    if (ctx->transport == VIRTIO_TRANSPORT_PCI) {
        virtio_write16(ctx, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
    } else {
        virtio_write32(ctx, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
    }

    // Poll for completion (simple busy wait)
    uint16_t last_used = ctx->queue.last_used_idx;
    int timeout = 100000;
    while (ctx->queue.used.idx == last_used && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        // Release the descriptor on timeout
        ctx->desc_in_use[desc_idx] = false;
        return -1;
    }

    // Get the used descriptor
    uint16_t used_idx = last_used % VIRTIO_RNG_QUEUE_SIZE;
    virtio_rng_used_elem_t *used_elem = &ctx->queue.used.ring[used_idx];
    uint32_t used_desc_id = used_elem->id;
    uint32_t bytes_read = used_elem->len;

    // Update last used index
    ctx->queue.last_used_idx++;

    // Release the descriptor
    if (used_desc_id < VIRTIO_RNG_QUEUE_SIZE) {
        ctx->desc_in_use[used_desc_id] = false;
    }

    // Copy data to user buffer
    if (bytes_read > size) {
        bytes_read = size;
    }

    memcpy(buffer, ctx->buffer, bytes_read);

    return (int)bytes_read;
}
