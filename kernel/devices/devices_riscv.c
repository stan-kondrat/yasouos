#include "devices.h"
#include "virtio_mmio.h"
#include "../platform/fdt_parser.h"
#include "../../common/common.h"

// PCIe ECAM (Enhanced Configuration Access Mechanism) base address
#define PCIE_ECAM_BASE          0x30000000ULL

// PCI configuration offsets
#define PCI_VENDOR_ID_OFFSET    0x00
#define PCI_DEVICE_ID_OFFSET    0x02
#define PCI_BAR0_OFFSET         0x10

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

            puts("[PCI] Device ");
            put_hex16(vendor);
            puts(":");
            put_hex16(device_val);
            puts(" at bus=");
            put_hex8(bus);
            puts(" dev=");
            put_hex8(device);
            putchar('\n');

            // Read BAR0 and BAR4 for later use
            uint32_t bar0 = pcie_ecam_read32(bus, device, 0, PCI_BAR0_OFFSET);
            uint32_t bar4 = pcie_ecam_read32(bus, device, 0, PCI_BAR0_OFFSET + 16);

            // Print all BARs with their types
            for (int i = 0; i < 6; i++) {
                uint32_t bar = pcie_ecam_read32(bus, device, 0, PCI_BAR0_OFFSET + (i * 4));
                uint64_t size = pci_probe_bar_size(bus, device, 0, PCI_BAR0_OFFSET + (i * 4));

                if (bar == 0) {
                    continue; // Skip unimplemented BARs
                }

                puts("  BAR");
                put_hex8(i);
                puts("=0x");
                put_hex32(bar);

                if (bar & 0x1) {
                    puts(" (I/O)");
                } else {
                    puts(" (MEM)");
                }

                puts(" size=0x");
                put_hex64(size);
                putchar('\n');
            }

            // For VirtIO PCI devices on RISC-V, use BAR4 (MMIO) instead of BAR0 (I/O)
            uint64_t reg_base = 0;
            uint64_t reg_size = 0;

            if (vendor == 0x1AF4 && (bar0 & 0x1)) {
                // VirtIO device with I/O BAR0 - use BAR4 for MMIO instead
                puts("  VirtIO device detected, using BAR4 for MMIO\n");
                uint64_t bar4_size = pci_probe_bar_size(bus, device, 0, PCI_BAR0_OFFSET + 16);
                reg_base = bar4 & 0xFFFFFFF0;
                reg_size = bar4_size;

                puts("  BAR4 reg_base=0x");
                put_hex64(reg_base);
                puts(" (bar4=0x");
                put_hex32(bar4);
                puts(" masked), size=0x");
                put_hex64(reg_size);
                putchar('\n');
            } else {
                // Regular PCI device - use BAR0
                uint64_t bar0_size = pci_probe_bar_size(bus, device, 0, PCI_BAR0_OFFSET);
                reg_base = bar0 & 0xFFFFFFF0;
                reg_size = bar0_size;

                puts("  BAR0 reg_base=0x");
                put_hex64(reg_base);
                puts(" (bar0=0x");
                put_hex32(bar0);
                puts(" masked), size=0x");
                put_hex64(reg_size);
                putchar('\n');
            }

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

    // Try PCIe ECAM first
    if (pci_detect_ecam()) {
        puts("[PCI] Using PCIe ECAM at 0x");
        put_hex64(PCIE_ECAM_BASE);
        putchar('\n');
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
