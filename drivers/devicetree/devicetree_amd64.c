#include "devicetree.h"
#include "../../include/common.h"

// AMD64 uses legacy PCI I/O ports as "device tree"
#define PCI_CONFIG_ADDRESS_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT    0xCFC
#define PCI_ENABLE_BIT          0x80000000
#define PCI_VENDOR_ID_OFFSET    0x00
#define PCI_DEVICE_ID_OFFSET    0x02
#define PCI_BAR0_OFFSET         0x10

// x86_64 I/O port operations
static inline void io_outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t io_inl(uint16_t port) {
    uint32_t result;
    __asm__ volatile ("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static uint32_t pci_make_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return PCI_ENABLE_BIT |
           (bus << 16) |
           (device << 11) |
           (function << 8) |
           (offset & 0xFC);
}

static uint16_t pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    io_outl(PCI_CONFIG_ADDRESS_PORT, address);

    uint32_t data = io_inl(PCI_CONFIG_DATA_PORT);
    uint8_t shift = (offset & 2) * 8;
    return (data >> shift) & 0xFFFF;
}

static uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    io_outl(PCI_CONFIG_ADDRESS_PORT, address);
    return io_inl(PCI_CONFIG_DATA_PORT);
}

int dt_init(void) {
    return 0;
}

int dt_enumerate_devices(dt_device_callback_t callback, void *context) {
    int device_count = 0;

    // Scan first 2 PCI buses for virtual machine environments
    for (uint8_t bus = 0; bus < 2; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            uint16_t vendor = pci_config_read16(bus, device, 0, PCI_VENDOR_ID_OFFSET);

            if (vendor == 0xFFFF) {
                continue;
            }

            uint16_t device_val = pci_config_read16(bus, device, 0, PCI_DEVICE_ID_OFFSET);
            uint32_t bar0 = pci_config_read32(bus, device, 0, PCI_BAR0_OFFSET);

            // AMD64/PCI doesn't use compatible strings - just vendor/device IDs
            dt_device_t dev_info = {
                .compatible = NULL,        // No compatible string for PCI
                .reg_base = bar0 & 0xFFFC, // I/O port base
                .reg_size = 0,             // Unknown for PCI
                .vendor_id = vendor,
                .device_id = device_val,
                .bus = bus,
                .device = device,
                .function = 0
            };

            if (callback) {
                callback(&dev_info, context);
            }

            device_count++;
        }
    }

    return device_count;
}

int dt_find_device([[maybe_unused]] const char *compatible,
                   [[maybe_unused]] dt_device_t *device) {
    // AMD64 doesn't support compatible string lookup
    // Drivers should match by vendor/device ID
    return 0;
}

const char *dt_get_device_name([[maybe_unused]] uint16_t vendor_id,
                               [[maybe_unused]] uint16_t device_id) {
    // Device names come from drivers, not device tree
    return NULL;
}
