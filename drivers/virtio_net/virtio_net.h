#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"

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
