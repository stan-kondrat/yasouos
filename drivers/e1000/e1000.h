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

/**
 * E1000 device context
 */
typedef struct {
    uint64_t mmio_base;
    bool initialized;
    uint8_t mac_addr[6];
} e1000_t;

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
