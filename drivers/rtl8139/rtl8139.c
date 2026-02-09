#include "rtl8139.h"
#include "../../common/common.h"
#include "../../common/log.h"

static log_tag_t *rtl_log;

#define PCI_VENDOR_ID_REALTEK 0x10EC
#define RTL8139_DEVICE_ID 0x8139

static const device_id_t rtl8139_id_table[] = {
    { NULL, PCI_VENDOR_ID_REALTEK, RTL8139_DEVICE_ID, "Realtek RTL8139 Fast Ethernet" },
    { NULL, 0, 0, NULL }
};

static inline uint8_t rtl8139_read8(rtl8139_t *ctx, uint32_t offset) {
    if (ctx->use_mmio) {
        volatile uint8_t *mmio = (volatile uint8_t *)(uintptr_t)ctx->mmio_base;
        return mmio[offset];
    }
#if defined(__x86_64__) || defined(__i386__)
    return io_inb((uint16_t)ctx->mmio_base + offset);
#else
    return 0;
#endif
}

static inline uint16_t rtl8139_read16(rtl8139_t *ctx, uint32_t offset) {
    if (ctx->use_mmio) {
        volatile uint16_t *mmio = (volatile uint16_t *)(uintptr_t)ctx->mmio_base;
        return mmio[offset / 2];
    }
#if defined(__x86_64__) || defined(__i386__)
    return io_inw((uint16_t)ctx->mmio_base + offset);
#else
    return 0;
#endif
}

static inline uint32_t rtl8139_read32(rtl8139_t *ctx, uint32_t offset) {
    if (ctx->use_mmio) {
        volatile uint32_t *mmio = (volatile uint32_t *)(uintptr_t)ctx->mmio_base;
        return mmio[offset / 4];
    }
#if defined(__x86_64__) || defined(__i386__)
    return io_inl((uint16_t)ctx->mmio_base + offset);
#else
    return 0;
#endif
}

static inline void rtl8139_write8(rtl8139_t *ctx, uint32_t offset, uint8_t value) {
    if (ctx->use_mmio) {
        volatile uint8_t *mmio = (volatile uint8_t *)(uintptr_t)ctx->mmio_base;
        mmio[offset] = value;
    }
#if defined(__x86_64__) || defined(__i386__)
    else {
        io_outb((uint16_t)ctx->mmio_base + offset, value);
    }
#endif
}

static inline void rtl8139_write16(rtl8139_t *ctx, uint32_t offset, uint16_t value) {
    if (ctx->use_mmio) {
        volatile uint16_t *mmio = (volatile uint16_t *)(uintptr_t)ctx->mmio_base;
        mmio[offset / 2] = value;
    }
#if defined(__x86_64__) || defined(__i386__)
    else {
        io_outw((uint16_t)ctx->mmio_base + offset, value);
    }
#endif
}

static inline void rtl8139_write32(rtl8139_t *ctx, uint32_t offset, uint32_t value) {
    if (ctx->use_mmio) {
        volatile uint32_t *mmio = (volatile uint32_t *)(uintptr_t)ctx->mmio_base;
        mmio[offset / 4] = value;
    }
#if defined(__x86_64__) || defined(__i386__)
    else {
        io_outl((uint16_t)ctx->mmio_base + offset, value);
    }
#endif
}

static void read_mac_address(rtl8139_t *ctx) {
    for (int i = 0; i < 6; i++) {
        ctx->mac_addr[i] = rtl8139_read8(ctx, RTL8139_MAC0 + i);
    }
}

#if defined(__x86_64__) || defined(__i386__)
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static void enable_bus_master(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t address = 0x80000000 | ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) | ((uint32_t)function << 8) | 0x04;
    io_outl(PCI_CONFIG_ADDRESS, address);
    uint32_t command = io_inl(PCI_CONFIG_DATA);
    if ((command & 0x04) == 0) {
        io_outl(PCI_CONFIG_DATA, command | 0x04);
    }
}
#endif

static int rtl8139_init_context(void *ctx, device_t *device) {
    if (!ctx || !device) {
        return -1;
    }

    if (!rtl_log) rtl_log = log_register("rtl8139", LOG_INFO);

    rtl8139_t *rtl_ctx = (rtl8139_t *)ctx;
    rtl_ctx->mmio_base = device->reg_base;
    rtl_ctx->tx_current = 0;
    rtl_ctx->use_mmio = (device->reg_base >= 0x10000);

#if defined(__x86_64__) || defined(__i386__)
    if (!rtl_ctx->use_mmio) {
        enable_bus_master(device->bus, device->device_num, device->function);
    }
#endif

    read_mac_address(rtl_ctx);

    rtl8139_write8(rtl_ctx, RTL8139_CONFIG1, 0x00);

    rtl8139_write8(rtl_ctx, RTL8139_CMD, RTL8139_CMD_RST);
    while ((rtl8139_read8(rtl_ctx, RTL8139_CMD) & RTL8139_CMD_RST) != 0);

    for (int i = 0; i < 6; i++) {
        rtl8139_write8(rtl_ctx, RTL8139_MAC0 + i, rtl_ctx->mac_addr[i]);
    }

    rtl8139_write32(rtl_ctx, RTL8139_MAR0, 0xFFFFFFFF);
    rtl8139_write32(rtl_ctx, RTL8139_MAR0 + 4, 0xFFFFFFFF);

    rtl8139_write8(rtl_ctx, RTL8139_CMD, RTL8139_CMD_RE | RTL8139_CMD_TE);

    uint32_t rcr = RTL8139_RCR_AAP | RTL8139_RCR_APM | RTL8139_RCR_AM |
                   RTL8139_RCR_AB | RTL8139_RCR_WRAP | RTL8139_RCR_RBLEN_8K |
                   (4 << 13) | (4 << 8);
    rtl8139_write32(rtl_ctx, RTL8139_RCR, rcr);

    uint32_t tcr = RTL8139_TCR_IFG_STD | (4 << 8) | 0x03000000;
    rtl8139_write32(rtl_ctx, RTL8139_TCR, tcr);

    rtl8139_write32(rtl_ctx, RTL8139_RXBUF, (uint32_t)(uintptr_t)rtl_ctx->rx_buffer);
    rtl8139_write16(rtl_ctx, RTL8139_CAPR, 0xFFF0);
    rtl8139_write32(rtl_ctx, 0x4C, 0);

    rtl8139_write8(rtl_ctx, RTL8139_CMD, RTL8139_CMD_RE | RTL8139_CMD_TE);
    rtl8139_write16(rtl_ctx, RTL8139_ISR, 0xFFFF);
    rtl8139_write16(rtl_ctx, RTL8139_IMR, RTL8139_INT_RXOK | RTL8139_INT_RXERR);

    rtl_ctx->initialized = true;
    log_info(rtl_log, "Driver initialized successfully\n");
    return 0;
}

static void rtl8139_deinit_context(void *ctx) {
    (void)ctx;
}

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

int rtl8139_receive(rtl8139_t *ctx, uint8_t *buffer, size_t buffer_size, size_t *received_length) {
    if (!ctx || !buffer || !received_length || !ctx->initialized) {
        return -1;
    }

    if ((rtl8139_read8(ctx, RTL8139_CMD) & RTL8139_CMD_BUFE) != 0) {
        return -1;
    }

    uint16_t offset = (rtl8139_read16(ctx, RTL8139_CAPR) + 16) % 8192;

    for (volatile int i = 0; i < 1000; i++);

    uint16_t packet_status = *(uint16_t *)(ctx->rx_buffer + offset);
    uint16_t packet_length = *(uint16_t *)(ctx->rx_buffer + offset + 2);

    if ((packet_status & 0x01) == 0 || packet_length < 4) {
        return -1;
    }

    uint16_t isr = rtl8139_read16(ctx, RTL8139_ISR);
    if (isr != 0) {
        rtl8139_write16(ctx, RTL8139_ISR, isr);
    }

    packet_length -= 4;
    if (packet_length > buffer_size) {
        return -1;
    }

    for (size_t i = 0; i < packet_length; i++) {
        buffer[i] = ctx->rx_buffer[(offset + 4 + i) % 8192];
    }

    *received_length = packet_length;
    offset = ((offset + packet_length + 4 + 3) & ~3) % 8192;
    rtl8139_write16(ctx, RTL8139_CAPR, (uint16_t)(offset - 16));

    return 0;
}

int rtl8139_transmit(rtl8139_t *ctx, const uint8_t *buffer, size_t length) {
    if (!ctx || !buffer || !ctx->initialized || length > 2048 || length < 1) {
        return -1;
    }

    uint8_t descriptor = ctx->tx_current;

    for (size_t i = 0; i < length; i++) {
        ctx->tx_buffer[i] = buffer[i];
    }

    rtl8139_write32(ctx, RTL8139_TXADDR0 + (descriptor * 4), (uint32_t)(uintptr_t)ctx->tx_buffer);
    rtl8139_write32(ctx, RTL8139_TXSTATUS0 + (descriptor * 4), (uint32_t)length);

    ctx->tx_current = (ctx->tx_current + 1) % 4;
    return 0;
}
