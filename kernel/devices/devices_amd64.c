#include "devices.h"
#include "../../common/common.h"
#include "../../common/log.h"

static log_tag_t *pci_log;

// PCI configuration offsets
#define PCI_VENDOR_ID_OFFSET    0x00
#define PCI_DEVICE_ID_OFFSET    0x02
#define PCI_COMMAND_OFFSET      0x04
#define PCI_BAR0_OFFSET         0x10

// PCI Command Register bits
#define PCI_COMMAND_MEMORY      0x0002
#define PCI_COMMAND_MASTER      0x0004

// Legacy PCI I/O ports
#define PCI_CONFIG_ADDRESS_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT    0xCFC
#define PCI_ENABLE_BIT          0x80000000

// PCIe ECAM (Enhanced Configuration Access Mechanism) base address
#define PCIE_ECAM_BASE          0xB0000000ULL

// PCI access method detection
static bool use_ecam = false;

// Legacy PCI I/O port access
static uint32_t pci_make_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return PCI_ENABLE_BIT |
           ((uint32_t)bus << 16) |
           ((uint32_t)device << 11) |
           ((uint32_t)function << 8) |
           (offset & 0xFC);
}

static uint16_t pci_io_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    io_outl(PCI_CONFIG_ADDRESS_PORT, address);
    uint32_t data = io_inl(PCI_CONFIG_DATA_PORT);
    uint8_t shift = (offset & 2) * 8;
    return (data >> shift) & 0xFFFF;
}

static uint32_t pci_io_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    io_outl(PCI_CONFIG_ADDRESS_PORT, address);
    return io_inl(PCI_CONFIG_DATA_PORT);
}

static void pci_io_write16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    io_outl(PCI_CONFIG_ADDRESS_PORT, address);
    uint32_t data = io_inl(PCI_CONFIG_DATA_PORT);
    uint8_t shift = (offset & 2) * 8;
    uint32_t mask = 0xFFFF << shift;
    data = (data & ~mask) | ((uint32_t)value << shift);
    io_outl(PCI_CONFIG_DATA_PORT, data);
}

static void pci_io_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    io_outl(PCI_CONFIG_ADDRESS_PORT, address);
    io_outl(PCI_CONFIG_DATA_PORT, value);
}

// PCIe ECAM memory-mapped access
// ECAM Address = Base + (Bus << 20) | (Device << 15) | (Function << 12) | Offset
static inline volatile uint32_t* pcie_ecam_address(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    uintptr_t addr = PCIE_ECAM_BASE |
                     ((uintptr_t)bus << 20) |
                     ((uintptr_t)device << 15) |
                     ((uintptr_t)function << 12) |
                     (offset & 0xFFC);
    return (volatile uint32_t*)addr;
}

static uint16_t pcie_ecam_read16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    volatile uint32_t* addr = pcie_ecam_address(bus, device, function, offset);
    uint32_t value = *addr;
    uint8_t shift = (offset & 2) * 8;
    return (value >> shift) & 0xFFFF;
}

static uint32_t pcie_ecam_read32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    volatile uint32_t* addr = pcie_ecam_address(bus, device, function, offset);
    return *addr;
}

static void pcie_ecam_write16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value) {
    volatile uint32_t* addr = pcie_ecam_address(bus, device, function, offset);
    uint32_t current = *addr;
    uint8_t shift = (offset & 2) * 8;
    uint32_t mask = 0xFFFF << shift;
    uint32_t new_value = (current & ~mask) | ((uint32_t)value << shift);
    *addr = new_value;
}

static void pcie_ecam_write32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value) {
    volatile uint32_t* addr = pcie_ecam_address(bus, device, function, offset);
    *addr = value;
}

// Unified PCI config access (dispatches to I/O ports or ECAM)
static uint16_t pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    if (use_ecam) {
        return pcie_ecam_read16(bus, device, function, offset);
    }
    return pci_io_read16(bus, device, function, offset);
}

static uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    if (use_ecam) {
        return pcie_ecam_read32(bus, device, function, offset);
    }
    return pci_io_read32(bus, device, function, offset);
}

static void pci_config_write16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value) {
    if (use_ecam) {
        pcie_ecam_write16(bus, device, function, offset, value);
    } else {
        pci_io_write16(bus, device, function, offset, value);
    }
}

static void pci_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value) {
    if (use_ecam) {
        pcie_ecam_write32(bus, device, function, offset, value);
    } else {
        pci_io_write32(bus, device, function, offset, value);
    }
}

// Detect if ECAM is available by probing the ECAM region
static bool pci_detect_ecam(void) {
    // ECAM detection disabled on AMD64 for now
    // Accessing 0xB0000000 without proper page table setup can cause faults
    // Legacy I/O ports (0xCF8/0xCFC) work reliably on all AMD64 platforms
    return false;
}

// Probe BAR size by writing all 1s and reading back
static uint64_t pci_probe_bar_size(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_offset) {
    // Read original BAR value
    uint32_t original_bar = pci_config_read32(bus, device, function, bar_offset);

    // Write all 1s to BAR
    pci_config_write32(bus, device, function, bar_offset, 0xFFFFFFFF);

    // Read back to get size mask
    uint32_t size_mask = pci_config_read32(bus, device, function, bar_offset);

    // Restore original BAR value
    pci_config_write32(bus, device, function, bar_offset, original_bar);

    // Calculate size (BAR address bits that can be modified)
    if (size_mask == 0 || size_mask == 0xFFFFFFFF) {
        return 0;
    }

    // For memory BARs, clear the bottom 4 bits (type/prefetchable flags)
    if ((original_bar & 0x1) == 0) {
        size_mask &= 0xFFFFFFF0;
    } else {
        // For I/O BARs, clear bottom 2 bits
        size_mask &= 0xFFFFFFFC;
    }

    // Size is the inverse of the mask + 1
    return (~size_mask) + 1;
}

int devices_enumerate(device_callback_t callback, void *context) {
    int device_count = 0;

    if (!pci_log) pci_log = log_register("pci", LOG_INFO);

    // Detect ECAM availability
    use_ecam = pci_detect_ecam();
    if (log_enabled(pci_log, LOG_DEBUG)) {
        if (use_ecam) {
            log_prefix(pci_log, LOG_DEBUG);
            puts("Using PCIe ECAM at 0x");
            put_hex64(PCIE_ECAM_BASE);
            putchar('\n');
        } else {
            log_prefix(pci_log, LOG_DEBUG);
            puts("Using legacy I/O ports (0x");
            put_hex16(PCI_CONFIG_ADDRESS_PORT);
            puts("/0x");
            put_hex16(PCI_CONFIG_DATA_PORT);
            puts(")\n");
        }
    }

    // Scan first 2 PCI buses for virtual machine environments
    for (uint8_t bus = 0; bus < 2; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            uint16_t vendor = pci_config_read16(bus, device, 0, PCI_VENDOR_ID_OFFSET);

            if (vendor == 0xFFFF) {
                continue;
            }

            uint16_t device_val = pci_config_read16(bus, device, 0, PCI_DEVICE_ID_OFFSET);
            uint32_t bar0 = pci_config_read32(bus, device, 0, PCI_BAR0_OFFSET);
            uint64_t bar_size = pci_probe_bar_size(bus, device, 0, PCI_BAR0_OFFSET);

            // Enable bus mastering and memory access for PCI device
            uint16_t command = pci_config_read16(bus, device, 0, PCI_COMMAND_OFFSET);
            command |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
            pci_config_write16(bus, device, 0, PCI_COMMAND_OFFSET, command);

            // AMD64/PCI doesn't use compatible strings - just vendor/device IDs
            device_t dev_info = {
                .compatible = NULL,
                .name = NULL,
                .reg_base = bar0 & 0xFFFFFFF0,
                .reg_size = bar_size,
                .vendor_id = vendor,
                .device_id = device_val,
                .bus = bus,
                .device_num = device,
                .function = 0,
                .driver = NULL,
                .state = DEVICE_STATE_DISCOVERED,
                .mmio_virt = NULL,
                .parent = NULL,
                .first_child = NULL,
                .next_sibling = NULL,
                .depth = 0,
                .next = NULL
            };

            if (callback) {
                callback(&dev_info, context);
            }

            device_count++;
        }
    }

    return device_count;
}

int devices_find([[maybe_unused]] const char *compatible,
                   [[maybe_unused]] device_t *device) {
    // AMD64 doesn't support compatible string lookup
    // Drivers should match by vendor/device ID
    return 0;
}

const char *devices_get_name([[maybe_unused]] uint16_t vendor_id,
                               [[maybe_unused]] uint16_t device_id) {
    // Device names come from drivers, not device tree
    return NULL;
}
