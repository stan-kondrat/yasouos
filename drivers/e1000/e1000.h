#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"

// Intel 82540EM Vendor and Device IDs
#define PCI_VENDOR_ID_INTEL     0x8086
#define E1000_DEVICE_ID_82540EM 0x100E

// E1000 MMIO Register Offsets
#define E1000_CTRL      0x00000  // Device Control
#define E1000_STATUS    0x00008  // Device Status
#define E1000_EECD      0x00010  // EEPROM/Flash Control
#define E1000_EERD      0x00014  // EEPROM Read
#define E1000_ICR       0x000C0  // Interrupt Cause Read
#define E1000_IMS       0x000D0  // Interrupt Mask Set
#define E1000_RCTL      0x00100  // Receive Control
#define E1000_TCTL      0x00400  // Transmit Control
#define E1000_RDBAL     0x02800  // RX Descriptor Base Low
#define E1000_RDBAH     0x02804  // RX Descriptor Base High
#define E1000_RDLEN     0x02808  // RX Descriptor Length
#define E1000_RDH       0x02810  // RX Descriptor Head
#define E1000_RDT       0x02818  // RX Descriptor Tail
#define E1000_TDBAL     0x03800  // TX Descriptor Base Low
#define E1000_TDBAH     0x03804  // TX Descriptor Base High
#define E1000_TDLEN     0x03808  // TX Descriptor Length
#define E1000_TDH       0x03810  // TX Descriptor Head
#define E1000_TDT       0x03818  // TX Descriptor Tail
#define E1000_RAL       0x05400  // Receive Address Low
#define E1000_RAH       0x05404  // Receive Address High

// Control Register Bits
#define E1000_CTRL_RST      (1 << 26)  // Device Reset
#define E1000_CTRL_ASDE     (1 << 5)   // Auto-Speed Detection Enable
#define E1000_CTRL_SLU      (1 << 6)   // Set Link Up

// Status Register Bits
#define E1000_STATUS_LU     (1 << 1)   // Link Up

// Receive Control Register Bits
#define E1000_RCTL_EN       (1 << 1)   // Receive Enable
#define E1000_RCTL_UPE      (1 << 3)   // Unicast Promiscuous Enable
#define E1000_RCTL_MPE      (1 << 4)   // Multicast Promiscuous Enable
#define E1000_RCTL_BAM      (1 << 15)  // Broadcast Accept Mode
#define E1000_RCTL_BSIZE_2K (0 << 16)  // Buffer Size 2048 bytes

// Transmit Control Register Bits
#define E1000_TCTL_EN       (1 << 1)   // Transmit Enable
#define E1000_TCTL_PSP      (1 << 3)   // Pad Short Packets

// RX Descriptor Status Bits
#define E1000_RXD_STAT_DD   (1 << 0)   // Descriptor Done
#define E1000_RXD_STAT_EOP  (1 << 1)   // End of Packet

// TX Descriptor Command Bits
#define E1000_TXD_CMD_EOP   (1 << 0)   // End of Packet
#define E1000_TXD_CMD_RS    (1 << 3)   // Report Status

// TX Descriptor Status Bits
#define E1000_TXD_STAT_DD   (1 << 0)   // Descriptor Done

// Number of RX/TX descriptors
#define E1000_NUM_RX_DESC   8
#define E1000_NUM_TX_DESC   8
#define E1000_RX_BUFFER_SIZE 2048
#define E1000_TX_BUFFER_SIZE 2048

/**
 * E1000 RX Descriptor
 */
typedef struct {
    uint64_t buffer_addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

/**
 * E1000 TX Descriptor
 */
typedef struct {
    uint64_t buffer_addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

/**
 * E1000 device context
 */
typedef struct {
    uint64_t mmio_base;
    bool initialized;
    uint8_t mac_addr[6];
    e1000_rx_desc_t rx_descs[E1000_NUM_RX_DESC] __attribute__((aligned(16)));
    uint8_t rx_buffers[E1000_NUM_RX_DESC][E1000_RX_BUFFER_SIZE] __attribute__((aligned(16)));
    uint16_t rx_current;
    e1000_tx_desc_t tx_descs[E1000_NUM_TX_DESC] __attribute__((aligned(16)));
    uint8_t tx_buffers[E1000_NUM_TX_DESC][E1000_TX_BUFFER_SIZE] __attribute__((aligned(16)));
    uint16_t tx_current;
} __attribute__((aligned(16))) e1000_t;

/**
 * Get e1000 driver descriptor
 * @return Pointer to driver descriptor
 */
const driver_t* e1000_get_driver(void);

/**
 * Get MAC address from e1000 device
 * @param ctx Device context from driver initialization
 * @param mac Buffer to store 6-byte MAC address
 * @return 0 on success, -1 on error
 */
int e1000_get_mac(e1000_t *ctx, uint8_t mac[6]);

/**
 * Receive a packet from e1000 device
 * @param ctx Device context from driver initialization
 * @param buffer Buffer to store received packet
 * @param buffer_size Size of the buffer
 * @param received_length Pointer to store received packet length
 * @return 0 on success, -1 on error
 */
int e1000_receive(e1000_t *ctx, uint8_t *buffer, size_t buffer_size, size_t *received_length);

/**
 * Transmit a packet to e1000 device
 * @param ctx Device context from driver initialization
 * @param buffer Buffer containing packet to transmit
 * @param length Length of packet to transmit
 * @return 0 on success, -1 on error
 */
int e1000_transmit(e1000_t *ctx, const uint8_t *buffer, size_t length);
