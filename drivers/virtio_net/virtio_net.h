#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"

// VirtIO-Net queue size (actual descriptors we use)
#define VIRTIO_NET_QUEUE_SIZE 16
// Maximum queue size for PCI legacy (device advertises 256)
#define VIRTIO_NET_MAX_QUEUE_SIZE 256
#define VIRTIO_NET_MAX_PACKET_SIZE 2048

// ARM64 alignment offset: VirtIO header (10 bytes) + Ethernet header (14 bytes) = 24 bytes
// Adding 2-byte offset makes IP header (at +24) land on 4-byte boundary
#if defined(__aarch64__)
#define VIRTIO_NET_RX_BUFFER_OFFSET 2
#else
#define VIRTIO_NET_RX_BUFFER_OFFSET 0
#endif

// Virtqueue descriptor flags
#define VRING_DESC_F_NEXT 1
#define VRING_DESC_F_WRITE 2

// Virtqueue descriptor
typedef struct {
    volatile uint64_t addr;
    volatile uint32_t len;
    volatile uint16_t flags;
    volatile uint16_t next;
} virtio_net_desc_t;

// Virtqueue available ring
typedef struct {
    volatile uint16_t flags;
    volatile uint16_t idx;
    volatile uint16_t ring[VIRTIO_NET_QUEUE_SIZE];
    volatile uint16_t used_event;
} virtio_net_avail_t;

// Virtqueue used ring element
typedef struct {
    volatile uint32_t id;
    volatile uint32_t len;
} virtio_net_used_elem_t;

// Virtqueue used ring
typedef struct {
    volatile uint16_t flags;
    volatile uint16_t idx;
    virtio_net_used_elem_t ring[VIRTIO_NET_QUEUE_SIZE];
    volatile uint16_t avail_event;
} virtio_net_used_t;

// Virtqueue structure (VirtIO legacy split queue layout)
// This structure must match the VirtIO spec exactly - no extra fields!
typedef struct {
    virtio_net_desc_t desc[VIRTIO_NET_QUEUE_SIZE];
    virtio_net_avail_t avail;
    uint8_t padding[4096 - (16 * VIRTIO_NET_QUEUE_SIZE) - (6 + 2 * VIRTIO_NET_QUEUE_SIZE)];
    virtio_net_used_t used;
} __attribute__((aligned(4096))) virtio_net_queue_t;

// PCI-specific queue structure with max descriptor count
// PCI legacy virtio expects the full queue size layout
typedef struct {
    virtio_net_desc_t desc[VIRTIO_NET_MAX_QUEUE_SIZE];
    // Available ring structure (inline instead of using typedef for sizing)
    struct {
        volatile uint16_t flags;
        volatile uint16_t idx;
        volatile uint16_t ring[VIRTIO_NET_MAX_QUEUE_SIZE];
        volatile uint16_t used_event;
    } avail;
    // Padding to align used ring to 4096 boundary
    // desc: 256*16=4096, avail: 4+512+2=518, total=4614
    // Next 4096 boundary is 8192, so padding = 8192-4614 = 3578
    uint8_t padding[8192 - (16 * VIRTIO_NET_MAX_QUEUE_SIZE) - (6 + 2 * VIRTIO_NET_MAX_QUEUE_SIZE)];
    // Used ring structure
    struct {
        volatile uint16_t flags;
        volatile uint16_t idx;
        struct {
            volatile uint32_t id;
            volatile uint32_t len;
        } ring[VIRTIO_NET_MAX_QUEUE_SIZE];
        volatile uint16_t avail_event;
    } used;
} __attribute__((aligned(4096))) virtio_net_queue_pci_t;

// VirtIO-Net header (legacy)
typedef struct {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
} virtio_net_hdr_t;

/**
 * VirtIO transport types
 */
typedef enum {
    VIRTIO_NET_TRANSPORT_MMIO = 0,  // Memory-mapped I/O (ARM64, RISC-V)
    VIRTIO_NET_TRANSPORT_PCI = 1     // PCI I/O ports (AMD64)
} virtio_net_transport_t;

/**
 * VirtIO network device context
 */
typedef struct {
    uint64_t io_base;
    bool initialized;
    virtio_net_transport_t transport;
    uint8_t mac_addr[6];
    // Use union to support both MMIO (small queue) and PCI (large queue)
    union {
        virtio_net_queue_t mmio_rx_queue;
        virtio_net_queue_pci_t pci_rx_queue;
    };
    union {
        virtio_net_queue_t mmio_tx_queue;
        virtio_net_queue_pci_t pci_tx_queue;
    };
    uint8_t rx_buffers[VIRTIO_NET_QUEUE_SIZE][VIRTIO_NET_MAX_PACKET_SIZE];
    uint8_t tx_buffers[VIRTIO_NET_QUEUE_SIZE][VIRTIO_NET_MAX_PACKET_SIZE];
    bool rx_desc_in_use[VIRTIO_NET_QUEUE_SIZE];
    bool tx_desc_in_use[VIRTIO_NET_QUEUE_SIZE];
    uint16_t rx_last_used_idx;
    uint16_t tx_last_used_idx;
} virtio_net_t;

/**
 * Get virtio-net driver descriptor
 * @return Pointer to driver descriptor
 */
const driver_t* virtio_net_get_driver(void);

/**
 * Get MAC address from virtio-net device
 * @param ctx Device context from driver initialization
 * @param mac Buffer to store 6-byte MAC address
 * @return 0 on success, -1 on error
 */
int virtio_net_get_mac(virtio_net_t *ctx, uint8_t mac[6]);

/**
 * Transmit packet through virtio-net device
 * @param ctx Device context from driver initialization
 * @param packet Pointer to packet data
 * @param length Packet length in bytes
 * @return 0 on success, -1 on error
 */
int virtio_net_transmit(virtio_net_t *ctx, const uint8_t *packet, size_t length);

/**
 * Receive packet from virtio-net device (polling)
 * @param ctx Device context from driver initialization
 * @param buffer Buffer to store received packet
 * @param buffer_size Size of receive buffer
 * @param received_length Pointer to store received packet length
 * @return 0 on success, -1 on error or no packet available
 */
int virtio_net_receive(virtio_net_t *ctx, uint8_t *buffer, size_t buffer_size, size_t *received_length);
