#include "devices.h"
#include "virtio_mmio.h"
#include "../platform/fdt_parser.h"
#include "../../common/common.h"
#include "../../common/log.h"

static log_tag_t *pci_log;

// PCIe ECAM (Enhanced Configuration Access Mechanism) base address
#define PCIE_ECAM_BASE          0x4010000000ULL

// PCI MMIO allocation range for devices without pre-assigned BARs
// QEMU virt machine has PCI MMIO window at 0x10000000-0x3EFEFFFF
#define PCI_MMIO_BASE           0x10000000ULL
#define PCI_MMIO_SIZE           0x10000000ULL  // 256 MB range

// PCI configuration offsets
#define PCI_VENDOR_ID_OFFSET    0x00
#define PCI_DEVICE_ID_OFFSET    0x02
#define PCI_COMMAND_OFFSET      0x04
#define PCI_BAR0_OFFSET         0x10
#define PCI_BAR1_OFFSET         0x14

// PCI Command Register bits
#define PCI_COMMAND_MEMORY      0x0002
#define PCI_COMMAND_MASTER      0x0004

// Global MMIO allocator state
static uint64_t next_mmio_address = PCI_MMIO_BASE;

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

// Probe BAR size by writing all 1s and reading back
static uint64_t pci_probe_bar_size(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_offset) {
    uint32_t original_bar = pcie_ecam_read32(bus, device, function, bar_offset);
    pcie_ecam_write32(bus, device, function, bar_offset, 0xFFFFFFFF);
    uint32_t size_mask = pcie_ecam_read32(bus, device, function, bar_offset);
    pcie_ecam_write32(bus, device, function, bar_offset, original_bar);

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

    return (~size_mask) + 1;
}

// Assign BAR address if not already configured
static uint32_t pci_assign_bar_if_needed(uint8_t bus, uint8_t device, uint8_t function,
                                          uint8_t bar_offset, uint64_t bar_size) {
    uint32_t bar_value = pcie_ecam_read32(bus, device, function, bar_offset);

    if ((bar_value & 0x1) == 0) {
        uint32_t addr = bar_value & 0xFFFFFFF0;
        if (addr != 0) {
            return bar_value;
        }

        if (bar_size > 0 && next_mmio_address + bar_size <= PCI_MMIO_BASE + PCI_MMIO_SIZE) {
            uint32_t new_bar = (uint32_t)next_mmio_address | (bar_value & 0xF);
            pcie_ecam_write32(bus, device, function, bar_offset, new_bar);
            next_mmio_address += bar_size;
            return new_bar;
        }
    }

    return bar_value;
}

// Detect if ECAM is available by probing the ECAM region
static bool pci_detect_ecam(void) {
    volatile uint32_t* ecam_test = (volatile uint32_t*)PCIE_ECAM_BASE;
    uint32_t test_value = *ecam_test;
    uint16_t vendor = test_value & 0xFFFF;
    return (vendor != 0x0000 && vendor != 0xFFFF);
}

// Enumerate PCIe devices via ECAM
static int pcie_enumerate_devices(device_callback_t callback, void *context) {
    int device_count = 0;

    // Scan first 2 PCI buses
    for (uint8_t bus = 0; bus < 2; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            uint16_t vendor = pcie_ecam_read16(bus, device, 0, PCI_VENDOR_ID_OFFSET);

            if (vendor == 0xFFFF || vendor == 0x0000) {
                continue;
            }

            uint16_t device_val = pcie_ecam_read16(bus, device, 0, PCI_DEVICE_ID_OFFSET);

            uint64_t bar0_size = pci_probe_bar_size(bus, device, 0, PCI_BAR0_OFFSET);
            uint64_t bar1_size = pci_probe_bar_size(bus, device, 0, PCI_BAR1_OFFSET);

            uint32_t bar0 = pci_assign_bar_if_needed(bus, device, 0, PCI_BAR0_OFFSET, bar0_size);
            uint32_t bar1 = pci_assign_bar_if_needed(bus, device, 0, PCI_BAR1_OFFSET, bar1_size);

            uint64_t reg_base;
            uint64_t reg_size;

            if ((bar0 & 0x1) && bar1_size > 0 && (bar1 & 0x1) == 0) {
                reg_base = bar1 & 0xFFFFFFF0;
                reg_size = bar1_size;
            } else {
                reg_base = bar0 & 0xFFFFFFF0;
                reg_size = bar0_size;
            }

            uint16_t command = pcie_ecam_read16(bus, device, 0, PCI_COMMAND_OFFSET);
            command |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
            pcie_ecam_write16(bus, device, 0, PCI_COMMAND_OFFSET, command);

            device_t dev_info = {
                .compatible = NULL,
                .name = NULL,
                .reg_base = reg_base,
                .reg_size = reg_size,
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

// Callback wrapper to probe VirtIO devices from FDT
static void probe_virtio_callback(const device_t *device, void *context) {
    // Only process virtio,mmio devices
    if (!device->compatible || strcmp(device->compatible, "virtio,mmio") != 0) {
        // Pass through non-VirtIO devices directly
        device_callback_t user_callback = (device_callback_t)context;
        if (user_callback) {
            user_callback(device, NULL);
        }
        return;
    }

    // Create a mutable copy to probe
    device_t mutable_device = *device;

    // Probe the VirtIO device to get vendor/device IDs
    if (virtio_mmio_probe_device(&mutable_device) == 0) {
        // Successfully probed - pass to user callback
        device_callback_t user_callback = (device_callback_t)context;
        if (user_callback) {
            user_callback(&mutable_device, NULL);
        }
    }
}

int devices_enumerate(device_callback_t callback, [[maybe_unused]] void *context) {
    int total_devices = 0;

    if (!pci_log) pci_log = log_register("pci", LOG_INFO);

    // Try PCIe ECAM first
    if (pci_detect_ecam()) {
        if (log_enabled(pci_log, LOG_DEBUG)) {
            log_prefix(pci_log, LOG_DEBUG);
            puts("Using PCIe ECAM at 0x");
            put_hex64(PCIE_ECAM_BASE);
            putchar('\n');
        }
        total_devices += pcie_enumerate_devices(callback, context);
    }

    // Also enumerate FDT devices
    uintptr_t fdt_addr = device_get_fdt();
    if (fdt_addr != 0) {
        total_devices += fdt_enumerate_devices(fdt_addr, probe_virtio_callback, (void*)callback);
    }

    return total_devices;
}

int devices_find([[maybe_unused]] const char *compatible,
                   [[maybe_unused]] device_t *device) {
    return 0;
}

const char *devices_get_name([[maybe_unused]] uint16_t vendor_id,
                               [[maybe_unused]] uint16_t device_id) {
    return NULL;
}
