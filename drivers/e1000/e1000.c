#include "e1000.h"

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
    volatile uint32_t *mmio = (volatile uint32_t *)(uintptr_t)ctx->mmio_base;
    uint32_t ral = mmio[E1000_RAL / 4];
    uint32_t rah = mmio[E1000_RAH / 4];

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

    e1000_t *e1000_ctx = (e1000_t *)ctx;
    e1000_ctx->mmio_base = device->reg_base;
    e1000_ctx->initialized = true;

    // Read MAC address from device
    e1000_read_mac_address(e1000_ctx);

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
