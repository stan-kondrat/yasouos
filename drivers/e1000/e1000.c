#include "e1000.h"
#include "../../common/common.h"
#include "../../common/log.h"

static log_tag_t *e1000_log;

// ARM64 requires special handling for packed structures with -O3
// GCC generates unaligned memory access instructions which can cause issues
// TODO: remove
#if defined(__aarch64__)
#pragma GCC optimize ("O0")
#endif

// Intel 82540EM Vendor and Device IDs
#define PCI_VENDOR_ID_INTEL     0x8086
#define E1000_DEVICE_ID_82540EM 0x100E

// Device ID table for matching
static const device_id_t e1000_id_table[] = {
    { NULL, PCI_VENDOR_ID_INTEL, E1000_DEVICE_ID_82540EM, "Intel 82540EM Gigabit Ethernet" },
    { NULL, 0, 0, NULL } // Terminator
};

// Use the public typedef from header
typedef e1000_t e1000_ctx_t;

// MMIO register access helpers
static inline uint32_t e1000_read32(e1000_t *ctx, uint32_t offset) {
    uint64_t addr = ctx->mmio_base + offset;
    volatile uint32_t *mmio = (volatile uint32_t *)(uintptr_t)addr;
    return *mmio;
}

static inline void e1000_write32(e1000_t *ctx, uint32_t offset, uint32_t value) {
    volatile uint32_t *mmio = (volatile uint32_t *)(uintptr_t)(ctx->mmio_base + offset);
    *mmio = value;
}

// Helper function to read MAC from EEPROM or registers
static void e1000_read_mac_address(e1000_t *ctx) {
    // For now, read from RAL/RAH registers
    // The E1000 82540EM stores MAC in EEPROM, but for simplicity we'll read from registers
    // which are typically initialized by the BIOS/firmware
    if (ctx->mmio_base == 0) {
        // No valid MMIO base, set to default MAC
        for (int i = 0; i < 6; i++) {
            ctx->mac_addr[i] = 0;
        }
        return;
    }

    // Read MAC address from RAL (Receive Address Low) and RAH (Receive Address High)
    uint32_t ral = e1000_read32(ctx, E1000_RAL);
    uint32_t rah = e1000_read32(ctx, E1000_RAH);

    ctx->mac_addr[0] = (ral >> 0) & 0xFF;
    ctx->mac_addr[1] = (ral >> 8) & 0xFF;
    ctx->mac_addr[2] = (ral >> 16) & 0xFF;
    ctx->mac_addr[3] = (ral >> 24) & 0xFF;
    ctx->mac_addr[4] = (rah >> 0) & 0xFF;
    ctx->mac_addr[5] = (rah >> 8) & 0xFF;
}

// Lifecycle hooks
static int e1000_init_context(void *ctx, device_t *device) {
    if (!ctx || !device) {
        return -1;
    }

    if (!e1000_log) e1000_log = log_register("e1000", LOG_INFO);

    e1000_t *e1000_ctx = (e1000_t *)ctx;
    e1000_ctx->mmio_base = device->reg_base;
    e1000_ctx->rx_current = 0;
    e1000_ctx->tx_current = 0;

    // Enable bus mastering and memory access in PCI command register (if needed)
    // This is typically done by the PCI enumeration code, but we ensure it here

    // Set link up
    uint32_t ctrl = e1000_read32(e1000_ctx, E1000_CTRL);
    ctrl |= E1000_CTRL_SLU;  // Set Link Up
    e1000_write32(e1000_ctx, E1000_CTRL, ctrl);

    // Small delay for link to stabilize
    for (volatile int i = 0; i < 100000; i++);

    // Read MAC address from device
    e1000_read_mac_address(e1000_ctx);

    // Initialize RX descriptors
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        e1000_ctx->rx_descs[i].buffer_addr = (uintptr_t)e1000_ctx->rx_buffers[i];
        e1000_ctx->rx_descs[i].status = 0;
    }

    // Set up RX descriptor ring
    e1000_write32(e1000_ctx, E1000_RDBAL, (uint32_t)(uintptr_t)e1000_ctx->rx_descs);
    e1000_write32(e1000_ctx, E1000_RDBAH, (uint32_t)((uintptr_t)e1000_ctx->rx_descs >> 32));
    e1000_write32(e1000_ctx, E1000_RDLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    e1000_write32(e1000_ctx, E1000_RDH, 0);
    e1000_write32(e1000_ctx, E1000_RDT, E1000_NUM_RX_DESC - 1);

    // Clear any pending interrupts
    e1000_read32(e1000_ctx, E1000_ICR);  // Read to clear

    // Disable interrupts (we're polling)
    e1000_write32(e1000_ctx, E1000_IMS, 0);

    // Note: Must enable transmitter BEFORE receiver for proper operation
    // Initialize TX descriptors first
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        e1000_ctx->tx_descs[i].buffer_addr = (uintptr_t)e1000_ctx->tx_buffers[i];
        e1000_ctx->tx_descs[i].status = E1000_TXD_STAT_DD;
        e1000_ctx->tx_descs[i].cmd = 0;
    }

    // Set up TX descriptor ring
    e1000_write32(e1000_ctx, E1000_TDBAL, (uint32_t)(uintptr_t)e1000_ctx->tx_descs);
    e1000_write32(e1000_ctx, E1000_TDBAH, (uint32_t)((uintptr_t)e1000_ctx->tx_descs >> 32));
    e1000_write32(e1000_ctx, E1000_TDLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_write32(e1000_ctx, E1000_TDH, 0);
    e1000_write32(e1000_ctx, E1000_TDT, 0);

    // Enable transmitter FIRST
    uint32_t tctl = E1000_TCTL_EN | E1000_TCTL_PSP;
    e1000_write32(e1000_ctx, E1000_TCTL, tctl);

    // Now enable receiver
    uint32_t rctl = E1000_RCTL_EN | E1000_RCTL_UPE | E1000_RCTL_MPE |
                    E1000_RCTL_BAM | E1000_RCTL_BSIZE_2K;
    e1000_write32(e1000_ctx, E1000_RCTL, rctl);

    e1000_ctx->initialized = true;
    log_info(e1000_log, "Driver initialized successfully\n");
    return 0;
}

static void e1000_deinit_context(void *ctx) {
    // TODO: Deinitialize E1000 device context
    (void)ctx;
}

// Driver descriptor
static const driver_t e1000_driver = {
    .name = "e1000",
    .version = "0.1.0",
    .type = DRIVER_TYPE_NETWORK,
    .id_table = e1000_id_table,
    .init_context = e1000_init_context,
    .deinit_context = e1000_deinit_context
};

const driver_t* e1000_get_driver(void) {
    return &e1000_driver;
}

int e1000_get_mac(e1000_t *ctx, uint8_t mac[6]) {
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

int e1000_receive(e1000_t *ctx, uint8_t *buffer, size_t buffer_size, size_t *received_length) {
    if (!ctx || !buffer || !received_length) {
        return -1;
    }

    if (!ctx->initialized) {
        return -1;
    }

    // Check current RX descriptor
    e1000_rx_desc_t *desc = &ctx->rx_descs[ctx->rx_current];

    // Check if descriptor has been used (DD bit set)
    if ((desc->status & E1000_RXD_STAT_DD) == 0) {
        // No packet available
        return -1;
    }

    // Check if this is end of packet
    if ((desc->status & E1000_RXD_STAT_EOP) == 0) {
        // Multi-descriptor packet not supported
        desc->status = 0;
        ctx->rx_current = (ctx->rx_current + 1) % E1000_NUM_RX_DESC;
        return -1;
    }

    // Get packet length
    uint16_t pkt_len = desc->length;

    // Check if buffer is large enough
    if (pkt_len > buffer_size) {
        desc->status = 0;
        ctx->rx_current = (ctx->rx_current + 1) % E1000_NUM_RX_DESC;
        return -1;
    }

    // Copy packet data
    for (size_t i = 0; i < pkt_len; i++) {
        buffer[i] = ctx->rx_buffers[ctx->rx_current][i];
    }

    *received_length = pkt_len;

    // Reset descriptor for reuse
    desc->status = 0;

    // Update RX tail pointer to make descriptor available again
    e1000_write32(ctx, E1000_RDT, ctx->rx_current);

    // Move to next descriptor
    ctx->rx_current = (ctx->rx_current + 1) % E1000_NUM_RX_DESC;

    return 0;
}

int e1000_transmit(e1000_t *ctx, const uint8_t *buffer, size_t length) {
    if (!ctx || !buffer) {
        return -1;
    }

    if (!ctx->initialized) {
        return -1;
    }

    if (length > E1000_TX_BUFFER_SIZE) {
        return -1;
    }

    // Get current TX descriptor
    e1000_tx_desc_t *desc = &ctx->tx_descs[ctx->tx_current];

    // Wait for descriptor to be free
    if ((desc->status & E1000_TXD_STAT_DD) == 0) {
        return -1;
    }

    // Copy packet to TX buffer
    for (size_t i = 0; i < length; i++) {
        ctx->tx_buffers[ctx->tx_current][i] = buffer[i];
    }

    // Set up descriptor
    desc->length = length;
    desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
    desc->status = 0;

    // Move to next descriptor
    uint16_t next_tx = (ctx->tx_current + 1) % E1000_NUM_TX_DESC;
    ctx->tx_current = next_tx;

    // Update tail pointer to trigger transmission
    e1000_write32(ctx, E1000_TDT, next_tx);

    return 0;
}
