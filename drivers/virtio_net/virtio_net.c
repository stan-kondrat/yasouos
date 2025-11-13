#include "virtio_net.h"
#include "../../kernel/devices/virtio_mmio.h"
#include "../../common/common.h"
#include "../../apps/network/protocols/ethernet/ethernet.h"

// VirtIO Vendor and Device IDs
#define VIRTIO_VENDOR_ID                0x1AF4
#define VIRTIO_NET_SUBSYSTEM_DEVICE_ID  0x0001  // Subsystem device ID for network
#define VIRTIO_NET_DEVICE_ID_TRANSITIONAL 0x1000  // Transitional device ID (0x1000 + subsystem device type)
#define VIRTIO_NET_DEVICE_ID_MODERN     0x1041  // Modern VirtIO 1.0+ device ID

// VirtIO-Net device-specific configuration space offsets
// For VirtIO-MMIO, device config starts at 0x100
#define VIRTIO_MMIO_CONFIG              0x100  // Device-specific config base
// For VirtIO-PCI legacy, device config starts at 0x14 (20 bytes)
#define VIRTIO_PCI_CONFIG               0x14   // Device-specific config base (PCI legacy)
#define VIRTIO_NET_CONFIG_MAC           0      // MAC address offset within device config

// Device ID table for matching
static const device_id_t virtio_net_id_table[] = {
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_MODERN, "VirtIO-Net (1.0+)" },
    { NULL, VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_MODERN, "VirtIO-Net (1.0+ PCI)" },
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_TRANSITIONAL, "VirtIO-Net (Transitional)" },
    { NULL, VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID_TRANSITIONAL, "VirtIO-Net (Transitional PCI)" },
    { "virtio,net", VIRTIO_VENDOR_ID, VIRTIO_NET_SUBSYSTEM_DEVICE_ID, "VirtIO-Net (Legacy)" },
    { NULL, VIRTIO_VENDOR_ID, VIRTIO_NET_SUBSYSTEM_DEVICE_ID, "VirtIO-Net (Legacy PCI)" },
    { 0, 0, 0, 0 } // Terminator
};

// Use the public typedef from header
typedef virtio_net_t virtio_net_ctx_t;

// Lifecycle hooks
static int virtio_net_init_context(void *ctx, device_t *device) {
    if (!ctx || !device) {
        return -1;
    }

    virtio_net_t *net_ctx = (virtio_net_t *)ctx;
    net_ctx->io_base = device->reg_base;
    net_ctx->initialized = true;

    // Detect transport type: PCI devices have low I/O port addresses (< 0x10000)
    // MMIO devices have high memory addresses
    if (net_ctx->io_base < 0x10000) {
        net_ctx->transport = VIRTIO_NET_TRANSPORT_PCI;
    } else {
        net_ctx->transport = VIRTIO_NET_TRANSPORT_MMIO;
    }

    // Device config offset depends on transport type
    uint64_t mac_offset = (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI)
                          ? VIRTIO_PCI_CONFIG
                          : VIRTIO_MMIO_CONFIG;

    // Read MAC address from device config
    for (int i = 0; i < 6; i++) {
#if defined(__x86_64__) || defined(__i386__)
        if (net_ctx->transport == VIRTIO_NET_TRANSPORT_PCI) {
            net_ctx->mac_addr[i] = io_inb((uint16_t)net_ctx->io_base + mac_offset + i);
        } else {
            net_ctx->mac_addr[i] = *(volatile uint8_t *)(uintptr_t)(net_ctx->io_base + mac_offset + i);
        }
#elif defined(__riscv) || defined(__aarch64__)
        // RISC-V/ARM64: Use MMIO access
        net_ctx->mac_addr[i] = *(volatile uint8_t *)(uintptr_t)(net_ctx->io_base + mac_offset + i);
#endif
    }

    return 0;
}

static void virtio_net_deinit_context(void *ctx) {
    // TODO: Deinitialize VirtIO network device context
    (void)ctx;
}

// Driver descriptor
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
    if (!ctx || !mac) {
        return -1;
    }

    if (!ctx->initialized) {
        return -1;
    }

    for (int i = 0; i < 6; i++) {
        mac[i] = ctx->mac_addr[i];
    }

    return 0;
}

// Simple packet queue for zero-copy transmission (per-device)
#define MAX_PACKET_SIZE 2048
#define PACKET_QUEUE_SIZE 8

typedef struct {
    uint8_t data[MAX_PACKET_SIZE];
    size_t length;
    bool occupied;
} packet_slot_t;

// Global queues (simplified - in real implementation would be per-device)
static packet_slot_t tx_queue[PACKET_QUEUE_SIZE] = {0};
static packet_slot_t rx_queue[PACKET_QUEUE_SIZE] = {0};

int virtio_net_transmit(virtio_net_t *ctx, const uint8_t *packet, size_t length) {
    if (!ctx || !packet || length == 0 || length > MAX_PACKET_SIZE) {
        return -1;
    }

    if (!ctx->initialized) {
        return -1;
    }

    // Print frame for debugging
    ethernet_print(packet, length);

    // Find free TX slot
    for (int i = 0; i < PACKET_QUEUE_SIZE; i++) {
        if (!tx_queue[i].occupied) {
            // Copy packet to TX queue
            for (size_t j = 0; j < length; j++) {
                tx_queue[i].data[j] = packet[j];
            }
            tx_queue[i].length = length;
            tx_queue[i].occupied = true;

            // Simulate hub broadcast: copy to multiple RX slots for all receivers
            int copies_queued = 0;
            for (int k = 0; k < PACKET_QUEUE_SIZE && copies_queued < 2; k++) {
                if (!rx_queue[k].occupied) {
                    for (size_t j = 0; j < length; j++) {
                        rx_queue[k].data[j] = packet[j];
                    }
                    rx_queue[k].length = length;
                    rx_queue[k].occupied = true;
                    copies_queued++;
                }
            }

            // Mark TX slot as free (packet sent)
            tx_queue[i].occupied = false;
            return 0;
        }
    }

    return -1;  // Queue full
}

int virtio_net_receive(virtio_net_t *ctx, uint8_t *buffer, size_t buffer_size, size_t *received_length) {
    if (!ctx || !buffer || !received_length) {
        return -1;
    }

    if (!ctx->initialized) {
        return -1;
    }

    // Poll RX queue for packets
    for (int i = 0; i < PACKET_QUEUE_SIZE; i++) {
        if (rx_queue[i].occupied) {
            if (rx_queue[i].length > buffer_size) {
                return -1;  // Buffer too small
            }

            // Copy packet from RX queue
            for (size_t j = 0; j < rx_queue[i].length; j++) {
                buffer[j] = rx_queue[i].data[j];
            }
            *received_length = rx_queue[i].length;

            // Print received frame
            ethernet_print(buffer, *received_length);

            // Mark slot as free
            rx_queue[i].occupied = false;
            return 0;
        }
    }

    return -1;  // No packets available
}
