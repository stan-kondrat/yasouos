#include "virtio_net.h"
#include "../../kernel/devices/virtio_mmio.h"
#include "../../common/common.h"
#include "../../apps/network/ethernet/ethernet.h"

// Helper macros to access the correct queue based on transport
#define GET_RX_DESC(ctx, idx) ((ctx)->transport == VIRTIO_NET_TRANSPORT_PCI ? \
    &(ctx)->pci_rx_queue.desc[idx] : &(ctx)->mmio_rx_queue.desc[idx])
#define GET_TX_DESC(ctx, idx) ((ctx)->transport == VIRTIO_NET_TRANSPORT_PCI ? \
    &(ctx)->pci_tx_queue.desc[idx] : &(ctx)->mmio_tx_queue.desc[idx])
#define GET_RX_AVAIL(ctx) ((ctx)->transport == VIRTIO_NET_TRANSPORT_PCI ? \
    &(ctx)->pci_rx_queue.avail : (void*)&(ctx)->mmio_rx_queue.avail)
#define GET_TX_AVAIL(ctx) ((ctx)->transport == VIRTIO_NET_TRANSPORT_PCI ? \
    &(ctx)->pci_tx_queue.avail : (void*)&(ctx)->mmio_tx_queue.avail)
#define GET_RX_USED(ctx) ((ctx)->transport == VIRTIO_NET_TRANSPORT_PCI ? \
    &(ctx)->pci_rx_queue.used : (void*)&(ctx)->mmio_rx_queue.used)
#define GET_TX_USED(ctx) ((ctx)->transport == VIRTIO_NET_TRANSPORT_PCI ? \
    &(ctx)->pci_tx_queue.used : (void*)&(ctx)->mmio_tx_queue.used)

// VirtIO Vendor and Device IDs
#define VIRTIO_VENDOR_ID                0x1AF4
#define VIRTIO_NET_SUBSYSTEM_DEVICE_ID  0x0001
#define VIRTIO_NET_DEVICE_ID_TRANSITIONAL 0x1000
#define VIRTIO_NET_DEVICE_ID_MODERN     0x1041

// VirtIO-Net device-specific configuration space offsets
#define VIRTIO_MMIO_CONFIG              0x100
#define VIRTIO_PCI_CONFIG               0x14
#define VIRTIO_NET_CONFIG_MAC           0

// Device ID table for matching
static const device_id_t virtio_net_id_table[] = {
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_MODERN, "VirtIO-Net (1.0+)" },
    { NULL, VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_MODERN, "VirtIO-Net (1.0+ PCI)" },
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_TRANSITIONAL, "VirtIO-Net (Transitional)" },
    { NULL, VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_TRANSITIONAL, "VirtIO-Net (Transitional PCI)" },
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_SUBSYSTEM_DEVICE_ID, "VirtIO-Net (Legacy)" },
    { NULL, VIRTIO_VENDOR_ID, VIRTIO_NET_SUBSYSTEM_DEVICE_ID, "VirtIO-Net (Legacy PCI)" },
    { 0, 0, 0, 0 }
};

typedef virtio_net_t virtio_net_ctx_t;

// Transport-agnostic register access helpers
static inline uint32_t virtio_read32(virtio_net_t *ctx, uint16_t offset) {
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        return io_inl((uint16_t)ctx->io_base + offset);
    }
#endif
    return mmio_read32(ctx->io_base + offset);
}

static inline uint16_t virtio_read16(virtio_net_t *ctx, uint16_t offset) {
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        return io_inw((uint16_t)ctx->io_base + offset);
    }
#endif
    return (uint16_t)mmio_read32(ctx->io_base + offset);
}

static inline uint8_t virtio_read8(virtio_net_t *ctx, uint16_t offset) {
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        return io_inb((uint16_t)ctx->io_base + offset);
    }
#endif
    return (uint8_t)mmio_read32(ctx->io_base + offset);
}

static inline void virtio_write32(virtio_net_t *ctx, uint16_t offset, uint32_t value) {
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        io_outl((uint16_t)ctx->io_base + offset, value);
        return;
    }
#endif
    mmio_write32(ctx->io_base + offset, value);
}

static inline void virtio_write16(virtio_net_t *ctx, uint16_t offset, uint16_t value) {
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        io_outw((uint16_t)ctx->io_base + offset, value);
        return;
    }
#endif
    mmio_write32(ctx->io_base + offset, value);
}

static inline void virtio_write8(virtio_net_t *ctx, uint16_t offset, uint8_t value) {
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        io_outb((uint16_t)ctx->io_base + offset, value);
        return;
    }
#endif
    mmio_write32(ctx->io_base + offset, value);
}

static int virtio_net_init_virtqueue(virtio_net_t *ctx, uint16_t queue_index) {
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        // PCI legacy virtio uses different register offsets
        virtio_write16(ctx, VIRTIO_PCI_QUEUE_SEL, queue_index);

        // Read max queue size
        uint32_t max_queue_size = virtio_read16(ctx, VIRTIO_PCI_QUEUE_NUM);
        if (max_queue_size == 0 || max_queue_size < VIRTIO_NET_QUEUE_SIZE) {
            return -1;
        }

        // For PCI legacy, use the full queue structure sized for max_queue_size
        // Zero out the appropriate queue
        void *queue_ptr;
        if (queue_index == 0) {
            memset(&ctx->pci_rx_queue, 0, sizeof(virtio_net_queue_pci_t));
            queue_ptr = &ctx->pci_rx_queue;
        } else {
            memset(&ctx->pci_tx_queue, 0, sizeof(virtio_net_queue_pci_t));
            queue_ptr = &ctx->pci_tx_queue;
        }

        uint64_t queue_addr = (uint64_t)queue_ptr;
        uint32_t queue_pfn = queue_addr >> 12;

        puts("[virtio-net] Queue addr: 0x");
        put_hex64(queue_addr);
        puts(" PFN: 0x");
        put_hex32(queue_pfn);
        puts("\n");

        virtio_write32(ctx, VIRTIO_PCI_QUEUE_PFN, queue_pfn);

        uint32_t readback_pfn = virtio_read32(ctx, VIRTIO_PCI_QUEUE_PFN);
        if (readback_pfn != queue_pfn) {
            puts("[virtio-net] ERROR: PFN mismatch! Expected: 0x");
            put_hex32(queue_pfn);
            puts(" Got: 0x");
            put_hex32(readback_pfn);
            puts("\n");
            return -1;
        }

        return 0;
    }
#endif

    // MMIO transport
    if (queue_index == 0) {
        memset(&ctx->mmio_rx_queue, 0, sizeof(virtio_net_queue_t));
    } else {
        memset(&ctx->mmio_tx_queue, 0, sizeof(virtio_net_queue_t));
    }

    virtio_write16(ctx, VIRTIO_MMIO_QUEUE_SEL, queue_index);

    uint32_t queue_size = virtio_read32(ctx, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (queue_size == 0 || queue_size < VIRTIO_NET_QUEUE_SIZE) {
        return -1;
    }

    virtio_write32(ctx, VIRTIO_MMIO_QUEUE_NUM, VIRTIO_NET_QUEUE_SIZE);
    virtio_write32(ctx, VIRTIO_MMIO_QUEUE_ALIGN, 4096);

    void *queue_ptr = (queue_index == 0) ? (void*)&ctx->mmio_rx_queue : (void*)&ctx->mmio_tx_queue;
    uint64_t queue_addr = (uint64_t)queue_ptr;
    uint32_t queue_pfn = queue_addr >> 12;

    puts("[virtio-net] Queue addr: 0x");
    put_hex64(queue_addr);
    puts(" PFN: 0x");
    put_hex32(queue_pfn);
    puts("\n");

    virtio_write32(ctx, VIRTIO_MMIO_QUEUE_PFN, queue_pfn);

    uint32_t readback_pfn = virtio_read32(ctx, VIRTIO_MMIO_QUEUE_PFN);
    if (readback_pfn != queue_pfn) {
        puts("[virtio-net] ERROR: PFN mismatch! Expected: 0x");
        put_hex32(queue_pfn);
        puts(" Got: 0x");
        put_hex32(readback_pfn);
        puts("\n");
        return -1;
    }

    return 0;
}

static int virtio_net_init_context(void *ctx, device_t *device) {
    if (!ctx || !device) {
        return -1;
    }

    virtio_net_t *net_ctx = (virtio_net_t *)ctx;
    memset(net_ctx, 0, sizeof(virtio_net_t));

    net_ctx->io_base = device->reg_base;
    net_ctx->transport = (net_ctx->io_base < 0x10000) ? VIRTIO_NET_TRANSPORT_PCI : VIRTIO_NET_TRANSPORT_MMIO;

#if defined(__x86_64__) || defined(__i386__)
    if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        // PCI legacy virtio initialization
        // Reset device
        virtio_write8(net_ctx, VIRTIO_PCI_STATUS, 0);

        // Set ACKNOWLEDGE status
        virtio_write8(net_ctx, VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);

        // Set DRIVER status
        virtio_write8(net_ctx, VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

        // Read device features and set driver features (accept no special features)
        (void)virtio_read32(net_ctx, VIRTIO_PCI_DEVICE_FEATURES);
        virtio_write32(net_ctx, VIRTIO_PCI_DRIVER_FEATURES, 0);

        // Set FEATURES_OK
        virtio_write8(net_ctx, VIRTIO_PCI_STATUS,
                      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

        // Verify FEATURES_OK
        uint8_t status = virtio_read8(net_ctx, VIRTIO_PCI_STATUS);
        if (!(status & VIRTIO_STATUS_FEATURES_OK)) {
            virtio_write8(net_ctx, VIRTIO_PCI_STATUS, VIRTIO_STATUS_FAILED);
            return -1;
        }
    } else
#endif
    {
        // MMIO transport initialization
        // Reset device
        virtio_write8(net_ctx, VIRTIO_MMIO_STATUS, 0);

        // Set ACKNOWLEDGE status
        virtio_write8(net_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);

        // Set DRIVER status
        virtio_write8(net_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

        // Read device features and set driver features (accept no special features)
        (void)virtio_read32(net_ctx, VIRTIO_MMIO_DEVICE_FEATURES);
        virtio_write32(net_ctx, VIRTIO_MMIO_DRIVER_FEATURES, 0);

        // Set FEATURES_OK
        virtio_write8(net_ctx, VIRTIO_MMIO_STATUS,
                      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

        // Verify FEATURES_OK
        uint8_t status = virtio_read8(net_ctx, VIRTIO_MMIO_STATUS);
        if (!(status & VIRTIO_STATUS_FEATURES_OK)) {
            virtio_write8(net_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
            return -1;
        }

        // Set guest page size
        virtio_write32(net_ctx, VIRTIO_MMIO_GUEST_PAGE_SIZE, 4096);
    }

    // Initialize RX queue (queue 0)
    puts("[virtio-net] Initializing RX queue...\n");
    if (virtio_net_init_virtqueue(net_ctx, 0) != 0) {
        puts("[virtio-net] ERROR: RX queue init failed\n");
#if defined(__x86_64__) || defined(__i386__)
        if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
            virtio_write8(net_ctx, VIRTIO_PCI_STATUS, VIRTIO_STATUS_FAILED);
        } else
#endif
        {
            virtio_write8(net_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        }
        return -1;
    }
    puts("[virtio-net] RX queue initialized\n");

    // Initialize TX queue (queue 1)
    puts("[virtio-net] Initializing TX queue...\n");
    if (virtio_net_init_virtqueue(net_ctx, 1) != 0) {
        puts("[virtio-net] ERROR: TX queue init failed\n");
#if defined(__x86_64__) || defined(__i386__)
        if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
            virtio_write8(net_ctx, VIRTIO_PCI_STATUS, VIRTIO_STATUS_FAILED);
        } else
#endif
        {
            virtio_write8(net_ctx, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        }
        return -1;
    }
    puts("[virtio-net] TX queue initialized\n");

    // Pre-populate RX queue with buffer descriptors
    // Apply alignment offset for ARM64 to ensure IP header 4-byte alignment
    for (uint16_t i = 0; i < VIRTIO_NET_QUEUE_SIZE; i++) {
        virtio_net_desc_t *desc = GET_RX_DESC(net_ctx, i);
        desc->addr = (uint64_t)&net_ctx->rx_buffers[i][VIRTIO_NET_RX_BUFFER_OFFSET];
        desc->len = VIRTIO_NET_MAX_PACKET_SIZE - VIRTIO_NET_RX_BUFFER_OFFSET;
        desc->flags = VRING_DESC_F_WRITE;
        desc->next = 0;

        // Access avail ring through helper
        if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
            uint16_t avail_idx = net_ctx->pci_rx_queue.avail.idx % VIRTIO_NET_QUEUE_SIZE;
            net_ctx->pci_rx_queue.avail.ring[avail_idx] = i;
            net_ctx->pci_rx_queue.avail.idx++;
        } else {
            uint16_t avail_idx = net_ctx->mmio_rx_queue.avail.idx % VIRTIO_NET_QUEUE_SIZE;
            net_ctx->mmio_rx_queue.avail.ring[avail_idx] = i;
            net_ctx->mmio_rx_queue.avail.idx++;
        }
        net_ctx->rx_desc_in_use[i] = true;
    }

    uint16_t rx_avail_idx = (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) ?
        net_ctx->pci_rx_queue.avail.idx : net_ctx->mmio_rx_queue.avail.idx;
    puts("[virtio-net] RX buffers populated, avail.idx=");
    put_hex16(rx_avail_idx);
    puts("\n");

    __sync_synchronize();

#if defined(__x86_64__) || defined(__i386__)
    if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        // PCI: Set DRIVER_OK (no FEATURES_OK in legacy)
        virtio_write8(net_ctx, VIRTIO_PCI_STATUS,
                      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);

        // Verify DRIVER_OK
        uint8_t status = virtio_read8(net_ctx, VIRTIO_PCI_STATUS);
        if (!(status & VIRTIO_STATUS_DRIVER_OK)) {
            return -1;
        }

        // For PCI legacy, don't notify RX queue initially
        // The device will start using RX buffers automatically once DRIVER_OK is set
    } else
#endif
    {
        // MMIO: Set DRIVER_OK
        virtio_write8(net_ctx, VIRTIO_MMIO_STATUS,
                      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                      VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

        // Verify DRIVER_OK
        uint8_t status = virtio_read8(net_ctx, VIRTIO_MMIO_STATUS);
        if (!(status & VIRTIO_STATUS_DRIVER_OK)) {
            return -1;
        }

        // Notify device about RX buffers (kick RX queue - queue 0)
        virtio_write32(net_ctx, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
    }

    // Read MAC address from device config
    uint64_t mac_offset = (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) ? VIRTIO_PCI_CONFIG : VIRTIO_MMIO_CONFIG;

    for (int i = 0; i < 6; i++) {
#if defined(__x86_64__) || defined(__i386__)
        if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
            net_ctx->mac_addr[i] = io_inb((uint16_t)net_ctx->io_base + mac_offset + i);
        } else {
            net_ctx->mac_addr[i] = *(volatile uint8_t *)(uintptr_t)(net_ctx->io_base + mac_offset + i);
        }
#elif defined(__riscv) || defined(__aarch64__)
        net_ctx->mac_addr[i] = *(volatile uint8_t *)(uintptr_t)(net_ctx->io_base + mac_offset + i);
#endif
    }

    net_ctx->initialized = true;

    puts("[virtio-net] Driver initialized successfully\n");

    return 0;
}

static void virtio_net_deinit_context(void *ctx) {
    if (!ctx) {
        return;
    }

    virtio_net_t *net_ctx = (virtio_net_t *)ctx;
    if (net_ctx->initialized) {
#if defined(__x86_64__) || defined(__i386__)
        if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
            virtio_write8(net_ctx, VIRTIO_PCI_STATUS, 0);
        } else
#endif
        {
            virtio_write8(net_ctx, VIRTIO_MMIO_STATUS, 0);
        }
        net_ctx->initialized = false;
    }
}

static const driver_t virtio_net_driver = {
    .name = "virtio-net",
    .version = "0.1.0",
    .type = DRIVER_TYPE_NETWORK,
    .id_table = virtio_net_id_table,
    .init_context = virtio_net_init_context,
    .deinit_context = virtio_net_deinit_context
};

const driver_t* virtio_net_get_driver(void) {
    return &virtio_net_driver;
}

int virtio_net_get_mac(virtio_net_t *ctx, uint8_t mac[6]) {
    if (!ctx || !mac || !ctx->initialized) {
        return -1;
    }

    for (int i = 0; i < 6; i++) {
        mac[i] = ctx->mac_addr[i];
    }

    return 0;
}

int virtio_net_receive(virtio_net_t *ctx, uint8_t *buffer, size_t buffer_size, size_t *received_length) {
    if (!ctx || !buffer || !received_length || !ctx->initialized) {
        return -1;
    }

    // Check if there are used buffers in the RX queue
    uint16_t last_used = ctx->rx_last_used_idx;
    __sync_synchronize();

    uint16_t used_ring_idx;
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        used_ring_idx = ctx->pci_rx_queue.used.idx;
    } else {
        used_ring_idx = ctx->mmio_rx_queue.used.idx;
    }

    static int debug_count = 0;
    if (debug_count < 5) {
        puts("[virtio-net] RX check: last_used=");
        put_hex16(last_used);
        puts(" used_idx=");
        put_hex16(used_ring_idx);
        puts("\n");
        debug_count++;
    }

    if (used_ring_idx == last_used) {
        return -1;
    }

    // Get the used descriptor
    uint16_t used_idx = last_used % VIRTIO_NET_QUEUE_SIZE;
    uint32_t desc_id, packet_len;
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        desc_id = ctx->pci_rx_queue.used.ring[used_idx].id;
        packet_len = ctx->pci_rx_queue.used.ring[used_idx].len;
    } else {
        desc_id = ctx->mmio_rx_queue.used.ring[used_idx].id;
        packet_len = ctx->mmio_rx_queue.used.ring[used_idx].len;
    }
    __sync_synchronize();

    if (desc_id >= VIRTIO_NET_QUEUE_SIZE) {
        ctx->rx_last_used_idx++;
        return -1;
    }

    if (packet_len == 0 || packet_len > VIRTIO_NET_MAX_PACKET_SIZE) {
        ctx->rx_last_used_idx++;
        return -1;
    }

    // VirtIO-Net legacy header is 10 bytes, skip it
    size_t hdr_len = sizeof(virtio_net_hdr_t);
    if (packet_len < hdr_len) {
        ctx->rx_last_used_idx++;
        return -1;
    }

    size_t data_len = packet_len - hdr_len;
    if (data_len > buffer_size) {
        ctx->rx_last_used_idx++;
        return -1;
    }

    // Copy packet data (skip header, apply alignment offset)
    const uint8_t *rx_data = ctx->rx_buffers[desc_id] + VIRTIO_NET_RX_BUFFER_OFFSET + hdr_len;

    memcpy(buffer, rx_data, data_len);
    *received_length = data_len;

    // Update last used index
    ctx->rx_last_used_idx++;

    // Re-add descriptor to available ring for next packet
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        uint16_t avail_idx = ctx->pci_rx_queue.avail.idx % VIRTIO_NET_QUEUE_SIZE;
        ctx->pci_rx_queue.avail.ring[avail_idx] = desc_id;
        __sync_synchronize();
        ctx->pci_rx_queue.avail.idx++;
        __sync_synchronize();
    } else {
        uint16_t avail_idx = ctx->mmio_rx_queue.avail.idx % VIRTIO_NET_QUEUE_SIZE;
        ctx->mmio_rx_queue.avail.ring[avail_idx] = desc_id;
        __sync_synchronize();
        ctx->mmio_rx_queue.avail.idx++;
        __sync_synchronize();
    }

    // Notify device (kick RX queue - queue 0)
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        virtio_write16(ctx, VIRTIO_PCI_QUEUE_SEL, 0);
        virtio_write16(ctx, VIRTIO_PCI_QUEUE_NOTIFY, 0);
    } else
#endif
    {
        virtio_write32(ctx, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
    }

    return 0;
}

int virtio_net_transmit(virtio_net_t *ctx, const uint8_t *packet, size_t length) {
    if (!ctx || !packet || length == 0 || length > VIRTIO_NET_MAX_PACKET_SIZE || !ctx->initialized) {
        return -1;
    }

    // Find free TX descriptor
    uint16_t desc_idx = 0;
    bool found = false;
    for (uint16_t i = 0; i < VIRTIO_NET_QUEUE_SIZE; i++) {
        if (!ctx->tx_desc_in_use[i]) {
            desc_idx = i;
            found = true;
            break;
        }
    }

    if (!found) {
        return -1;
    }

    ctx->tx_desc_in_use[desc_idx] = true;

    // Build TX buffer with VirtIO header + packet
    // Apply alignment offset for ARM64 to ensure consistent buffer layout
    uint8_t *tx_buffer = ctx->tx_buffers[desc_idx] + VIRTIO_NET_RX_BUFFER_OFFSET;
    virtio_net_hdr_t *hdr = (virtio_net_hdr_t *)tx_buffer;
    memset(hdr, 0, sizeof(virtio_net_hdr_t));

    // Copy packet after header
    for (size_t i = 0; i < length; i++) {
        tx_buffer[sizeof(virtio_net_hdr_t) + i] = packet[i];
    }

    size_t total_len = sizeof(virtio_net_hdr_t) + length;

    // Setup descriptor
    virtio_net_desc_t *desc = GET_TX_DESC(ctx, desc_idx);
    desc->addr = (uint64_t)tx_buffer;
    desc->len = total_len;
    desc->flags = 0;
    desc->next = 0;

    // Add to available ring
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        uint16_t avail_idx = ctx->pci_tx_queue.avail.idx % VIRTIO_NET_QUEUE_SIZE;
        ctx->pci_tx_queue.avail.ring[avail_idx] = desc_idx;
        __sync_synchronize();
        ctx->pci_tx_queue.avail.idx++;
        __sync_synchronize();
    } else {
        uint16_t avail_idx = ctx->mmio_tx_queue.avail.idx % VIRTIO_NET_QUEUE_SIZE;
        ctx->mmio_tx_queue.avail.ring[avail_idx] = desc_idx;
        __sync_synchronize();
        ctx->mmio_tx_queue.avail.idx++;
        __sync_synchronize();
    }

    // Notify device (kick TX queue - queue 1)
#if defined(__x86_64__) || defined(__i386__)
    if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
        virtio_write16(ctx, VIRTIO_PCI_QUEUE_SEL, 1);
        virtio_write16(ctx, VIRTIO_PCI_QUEUE_NOTIFY, 1);
    } else
#endif
    {
        virtio_write32(ctx, VIRTIO_MMIO_QUEUE_NOTIFY, 1);
    }

    // Poll for completion
    uint16_t last_used = ctx->tx_last_used_idx;
    int timeout = 100000;
    while (timeout > 0) {
        __sync_synchronize();
        uint16_t current_used;
        if (ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
            current_used = ctx->pci_tx_queue.used.idx;
        } else {
            current_used = ctx->mmio_tx_queue.used.idx;
        }
        if (current_used != last_used) {
            break;
        }
        timeout--;
    }

    if (timeout == 0) {
        ctx->tx_desc_in_use[desc_idx] = false;
        return -1;
    }

    // Update last used index and free descriptor
    ctx->tx_last_used_idx++;
    ctx->tx_desc_in_use[desc_idx] = false;

    return 0;
}
