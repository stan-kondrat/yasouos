#include "rtl8139.h"

// Realtek RTL8139 Vendor and Device IDs
#define PCI_VENDOR_ID_REALTEK   0x10EC
#define RTL8139_DEVICE_ID       0x8139

// Device ID table for matching
static const device_id_t rtl8139_id_table[] = {
    { NULL, PCI_VENDOR_ID_REALTEK, RTL8139_DEVICE_ID, "Realtek RTL8139 Fast Ethernet" },
    { NULL, 0, 0, NULL } // Terminator
};

// Use the public typedef from header
typedef rtl8139_t rtl8139_ctx_t;

// x86_64 I/O port operations
static inline uint8_t io_inb(uint16_t port) {
#if defined(__x86_64__) || defined(__i386__)
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
#else
    (void)port;
    return 0;
#endif
}

// Helper function to read MAC address from device registers
static void rtl8139_read_mac_address(rtl8139_t *ctx) {
    if (ctx->mmio_base == 0) {
        // No valid MMIO base, set to default MAC
        for (int i = 0; i < 6; i++) {
            ctx->mac_addr[i] = 0;
        }
        return;
    }

    // RTL8139 stores MAC address in the first 6 bytes of registers
    // Read MAC address from MAC0 register (offset 0x00)
    // On x86, RTL8139 typically uses I/O ports; otherwise MMIO
#if defined(__x86_64__) || defined(__i386__)
    // Check if this is an I/O port address (< 0x10000)
    if (ctx->mmio_base < 0x10000) {
        // I/O port access
        for (int i = 0; i < 6; i++) {
            ctx->mac_addr[i] = io_inb((uint16_t)(ctx->mmio_base + RTL8139_MAC0 + i));
        }
        return;
    }
#endif

    // MMIO access
    volatile uint8_t *mmio = (volatile uint8_t *)(uintptr_t)ctx->mmio_base;
    for (int i = 0; i < 6; i++) {
        ctx->mac_addr[i] = mmio[RTL8139_MAC0 + i];
    }
}

// Lifecycle hooks
static int rtl8139_init_context(void *ctx, device_t *device) {
    if (!ctx || !device) {
        return -1;
    }

    rtl8139_t *rtl_ctx = (rtl8139_t *)ctx;
    rtl_ctx->mmio_base = device->reg_base;
    rtl_ctx->initialized = true;

    // Read MAC address from device
    rtl8139_read_mac_address(rtl_ctx);

    return 0;
}

static void rtl8139_deinit_context(void *ctx) {
    // TODO: Deinitialize RTL8139 device context
    (void)ctx;
}

// Driver descriptor
static const driver_t rtl8139_driver = {
    .name = "rtl8139",
    .version = "0.1.0",
    .type = DRIVER_TYPE_NETWORK,
    .id_table = rtl8139_id_table,
    .init_context = rtl8139_init_context,
    .deinit_context = rtl8139_deinit_context
};

const driver_t* rtl8139_get_driver(void) {
    return &rtl8139_driver;
}

int rtl8139_get_mac(rtl8139_t *ctx, uint8_t mac[6]) {
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
