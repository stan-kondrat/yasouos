#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"

// Realtek RTL8139 Vendor and Device IDs
#define PCI_VENDOR_ID_REALTEK   0x10EC
#define RTL8139_DEVICE_ID       0x8139

// RTL8139 MMIO Register Offsets
#define RTL8139_MAC0            0x00    // MAC Address (6 bytes)
#define RTL8139_MAR0            0x08    // Multicast Address (8 bytes)
#define RTL8139_TXSTATUS0       0x10    // TX Status (4 registers)
#define RTL8139_TXADDR0         0x20    // TX Address (4 registers)
#define RTL8139_RXBUF           0x30    // RX Buffer Address
#define RTL8139_CMD             0x37    // Command Register
#define RTL8139_CAPR            0x38    // Current Address of Packet Read
#define RTL8139_CBR             0x3A    // Current Buffer Register
#define RTL8139_IMR             0x3C    // Interrupt Mask Register
#define RTL8139_ISR             0x3E    // Interrupt Status Register
#define RTL8139_TCR             0x40    // Transmit Configuration
#define RTL8139_RCR             0x44    // Receive Configuration
#define RTL8139_CONFIG1         0x52    // Configuration Register 1

// Command Register Bits
#define RTL8139_CMD_RST         (1 << 4)  // Reset
#define RTL8139_CMD_RE          (1 << 3)  // Receiver Enable
#define RTL8139_CMD_TE          (1 << 2)  // Transmitter Enable
#define RTL8139_CMD_BUFE        (1 << 0)  // Buffer Empty

// Interrupt Status/Mask Bits
#define RTL8139_INT_RXOK        (1 << 0)  // Receive OK
#define RTL8139_INT_RXERR       (1 << 1)  // Receive Error
#define RTL8139_INT_TXOK        (1 << 2)  // Transmit OK
#define RTL8139_INT_TXERR       (1 << 3)  // Transmit Error
#define RTL8139_INT_RXOVW       (1 << 4)  // RX Buffer Overflow
#define RTL8139_INT_LINKCHG     (1 << 5)  // Link Change
#define RTL8139_INT_FOVW        (1 << 6)  // RX FIFO Overflow
#define RTL8139_INT_LENCHG      (1 << 13) // Cable Length Change
#define RTL8139_INT_TIMEOUT     (1 << 14) // Time Out

// Receive Configuration Register Bits
#define RTL8139_RCR_AAP         (1 << 0)  // Accept All Packets
#define RTL8139_RCR_APM         (1 << 1)  // Accept Physical Match
#define RTL8139_RCR_AM          (1 << 2)  // Accept Multicast
#define RTL8139_RCR_AB          (1 << 3)  // Accept Broadcast
#define RTL8139_RCR_WRAP        (1 << 7)  // Wrap
#define RTL8139_RCR_RBLEN_8K    (0 << 11) // RX Buffer Length 8K
#define RTL8139_RCR_RBLEN_16K   (1 << 11) // RX Buffer Length 16K
#define RTL8139_RCR_RBLEN_32K   (2 << 11) // RX Buffer Length 32K
#define RTL8139_RCR_RBLEN_64K   (3 << 11) // RX Buffer Length 64K

// Transmit Configuration Register Bits
#define RTL8139_TCR_CLRABT      (1 << 0)  // Clear Abort
#define RTL8139_TCR_IFG_STD     (3 << 24) // Interframe Gap Standard

// Transmit Status Register Bits
#define RTL8139_TSD_OWN         (1 << 13) // DMA operation completed
#define RTL8139_TSD_TOK         (1 << 15) // Transmit OK

// RX Buffer size (8KB + 16 bytes for wrap + 1.5KB for overflow)
#define RTL8139_RX_BUFFER_SIZE  (8192 + 16 + 1536)

/**
 * RTL8139 device context
 */
typedef struct {
    uint64_t mmio_base;
    bool initialized;
    bool use_mmio;
    uint8_t mac_addr[6];
    uint16_t rx_offset;
    volatile uint8_t rx_buffer[RTL8139_RX_BUFFER_SIZE] __attribute__((aligned(16)));
    uint8_t tx_current;
    uint8_t tx_buffer[2048] __attribute__((aligned(8)));
} __attribute__((aligned(16))) rtl8139_t;

/**
 * Get rtl8139 driver descriptor
 * @return Pointer to driver descriptor
 */
const driver_t* rtl8139_get_driver(void);

/**
 * Get MAC address from rtl8139 device
 * @param ctx Device context from driver initialization
 * @param mac Buffer to store 6-byte MAC address
 * @return 0 on success, -1 on error
 */
int rtl8139_get_mac(rtl8139_t *ctx, uint8_t mac[6]);

/**
 * Receive a packet from rtl8139 device
 * @param ctx Device context from driver initialization
 * @param buffer Buffer to store received packet
 * @param buffer_size Size of the buffer
 * @param received_length Pointer to store received packet length
 * @return 0 on success, -1 on error
 */
int rtl8139_receive(rtl8139_t *ctx, uint8_t *buffer, size_t buffer_size, size_t *received_length);

/**
 * Transmit a packet using rtl8139 device
 * @param ctx Device context from driver initialization
 * @param buffer Buffer containing packet to transmit
 * @param length Length of the packet
 * @return 0 on success, -1 on error
 */
int rtl8139_transmit(rtl8139_t *ctx, const uint8_t *buffer, size_t length);
