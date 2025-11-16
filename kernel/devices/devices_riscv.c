#include "devices.h"
#include "virtio_mmio.h"
#include "../platform/fdt_parser.h"
#include "../../common/common.h"

// PCIe ECAM (Enhanced Configuration Access Mechanism) base address
#define PCIE_ECAM_BASE          0x30000000ULL

// PCI MMIO allocation range (for devices without pre-assigned BARs)
#define PCI_MMIO_BASE           0x40000000ULL
#define PCI_MMIO_SIZE           0x10000000ULL  // 256 MB range

// PCI configuration offsets
#define PCI_VENDOR_ID_OFFSET    0x00
#define PCI_DEVICE_ID_OFFSET    0x02
#define PCI_COMMAND_OFFSET      0x04
#define PCI_BAR0_OFFSET         0x10
#define PCI_BAR1_OFFSET         0x14

// PCI Command Register bits
#define PCI_COMMAND_IO          0x0001  // Enable I/O space
#define PCI_COMMAND_MEMORY      0x0002  // Enable Memory space
#define PCI_COMMAND_MASTER      0x0004  // Enable Bus Master

// PCI constants
#define PCI_MAX_BUSES           2
#define PCI_MAX_DEVICES         32
#define PCI_MAX_BARS            6
#define PCI_INVALID_VENDOR_LOW  0x0000
#define PCI_INVALID_VENDOR_HIGH 0xFFFF
#define PCI_BAR_IO_FLAG         0x1
#define PCI_BAR_MEM_MASK        0xFFFFFFF0
#define PCI_BAR_IO_MASK         0xFFFFFFFC
#define PCI_VIRTIO_VENDOR_ID    0x1AF4
#define PCI_BAR4_OFFSET         (PCI_BAR0_OFFSET + 16)

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

static void pcie_ecam_write32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value) {
    volatile uint32_t* addr = pcie_ecam_address(bus, device, function, offset);
    *addr = value;
}

static void pcie_ecam_write16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value) {
    volatile uint32_t* addr = pcie_ecam_address(bus, device, function, offset & ~0x3);
    uint32_t current = *addr;
    uint8_t shift = (offset & 2) * 8;
    uint32_t mask = ~(0xFFFF << shift);
    *addr = (current & mask) | ((uint32_t)value << shift);
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

// Assign BAR address, always reassigning to avoid QEMU overlaps
static uint32_t pci_assign_bar_if_needed(uint8_t bus, uint8_t device, uint8_t function,
                                          uint8_t bar_offset, uint64_t bar_size) {
    uint32_t bar_value = pcie_ecam_read32(bus, device, function, bar_offset);

    if ((bar_value & 0x1) != 0) {
        return bar_value;
    }

    if (bar_size == 0) {
        return bar_value;
    }

    if (next_mmio_address + bar_size > PCI_MMIO_BASE + PCI_MMIO_SIZE) {
        return bar_value;
    }

    uint64_t aligned_address = (next_mmio_address + (bar_size - 1)) & ~(bar_size - 1);
    if (aligned_address + bar_size > PCI_MMIO_BASE + PCI_MMIO_SIZE) {
        return bar_value;
    }

    uint32_t new_bar = (uint32_t)aligned_address | (bar_value & 0xF);
    pcie_ecam_write32(bus, device, function, bar_offset, new_bar);
    next_mmio_address = aligned_address + bar_size;
    return new_bar;
}

// Detect if ECAM is available by probing the ECAM region
static bool pci_detect_ecam(void) {
    volatile uint32_t* ecam_test = (volatile uint32_t*)PCIE_ECAM_BASE;
    uint32_t test_value = *ecam_test;
    uint16_t vendor = test_value & 0xFFFF;
    return (vendor != 0x0000 && vendor != 0xFFFF);
}

static bool pci_vendor_is_valid(uint16_t vendor_id) {
    return vendor_id != PCI_INVALID_VENDOR_LOW && vendor_id != PCI_INVALID_VENDOR_HIGH;
}

static bool pci_bar_is_implemented(uint32_t bar_value) {
    return bar_value != 0;
}

static bool pci_bar_is_io(uint32_t bar_value) {
    return (bar_value & PCI_BAR_IO_FLAG) != 0;
}

static uint64_t pci_bar_get_address(uint32_t bar_value) {
    if (pci_bar_is_io(bar_value)) {
        return bar_value & PCI_BAR_IO_MASK;
    }
    return bar_value & PCI_BAR_MEM_MASK;
}

static const char* pci_bar_get_type_string(uint32_t bar_value) {
    return pci_bar_is_io(bar_value) ? " (I/O)" : " (MEM)";
}

static void pci_print_device_header(uint16_t vendor_id, uint16_t device_id, uint8_t bus, uint8_t device_num) {
    puts("[PCI] Device ");
    put_hex16(vendor_id);
    puts(":");
    put_hex16(device_id);
    puts(" at bus=");
    put_hex8(bus);
    puts(" dev=");
    put_hex8(device_num);
    putchar('\n');
}

static void pci_print_bar_info(uint8_t bar_index, uint32_t bar_value, uint64_t bar_size) {
    puts("  BAR");
    put_hex8(bar_index);
    puts("=0x");
    put_hex32(bar_value);
    puts(pci_bar_get_type_string(bar_value));
    puts(" size=0x");
    put_hex64(bar_size);
    putchar('\n');
}

static void pci_print_selected_bar(const char* bar_name, uint64_t base_address, uint32_t original_bar, uint64_t size) {
    puts("  ");
    puts(bar_name);
    puts(" reg_base=0x");
    put_hex64(base_address);
    puts(" (");
    puts(bar_name);
    puts("=0x");
    put_hex32(original_bar);
    puts(" masked), size=0x");
    put_hex64(size);
    putchar('\n');
}

typedef struct {
    uint32_t bar_value;
    uint64_t bar_size;
    uint8_t bar_index;
} pci_bar_info_t;

static pci_bar_info_t pci_read_bar(uint8_t bus, uint8_t device_num, uint8_t bar_index) {
    uint16_t bar_offset = PCI_BAR0_OFFSET + (bar_index * 4);
    uint64_t bar_size = pci_probe_bar_size(bus, device_num, 0, bar_offset);
    uint32_t bar_value = pcie_ecam_read32(bus, device_num, 0, bar_offset);

    return (pci_bar_info_t){
        .bar_value = bar_value,
        .bar_size = bar_size,
        .bar_index = bar_index
    };
}

static void pci_print_all_bars(uint8_t bus, uint8_t device_num) {
    for (uint8_t bar_index = 0; bar_index < PCI_MAX_BARS; bar_index++) {
        pci_bar_info_t bar_info = pci_read_bar(bus, device_num, bar_index);

        if (!pci_bar_is_implemented(bar_info.bar_value)) {
            continue;
        }

        pci_print_bar_info(bar_info.bar_index, bar_info.bar_value, bar_info.bar_size);
    }
}

static bool pci_is_virtio_with_io_bar(uint16_t vendor_id, uint32_t bar0_value) {
    return vendor_id == PCI_VIRTIO_VENDOR_ID && pci_bar_is_io(bar0_value);
}

typedef struct {
    uint64_t base_address;
    uint64_t size;
} pci_selected_bar_t;

static pci_selected_bar_t pci_select_bar_for_device([[maybe_unused]] uint8_t bus,
                                                      [[maybe_unused]] uint8_t device_num,
                                                      uint16_t vendor_id,
                                                      [[maybe_unused]] uint16_t device_id,
                                                      const pci_bar_info_t* bar0,
                                                      const pci_bar_info_t* bar1,
                                                      const pci_bar_info_t* bar4) {
    if (pci_is_virtio_with_io_bar(vendor_id, bar0->bar_value)) {
        puts("  VirtIO device detected, using BAR4 for MMIO\n");
        pci_print_selected_bar("BAR4", pci_bar_get_address(bar4->bar_value), bar4->bar_value, bar4->bar_size);

        return (pci_selected_bar_t){
            .base_address = pci_bar_get_address(bar4->bar_value),
            .size = bar4->bar_size
        };
    }

    if (pci_bar_is_io(bar0->bar_value) && bar1->bar_size > 0 && !pci_bar_is_io(bar1->bar_value)) {
        puts("  BAR0 is I/O, using BAR1 for MMIO\n");
        pci_print_selected_bar("BAR1", pci_bar_get_address(bar1->bar_value), bar1->bar_value, bar1->bar_size);

        return (pci_selected_bar_t){
            .base_address = pci_bar_get_address(bar1->bar_value),
            .size = bar1->bar_size
        };
    }

    pci_print_selected_bar("BAR0", pci_bar_get_address(bar0->bar_value), bar0->bar_value, bar0->bar_size);

    return (pci_selected_bar_t){
        .base_address = pci_bar_get_address(bar0->bar_value),
        .size = bar0->bar_size
    };
}

static device_t pci_create_device_info(uint8_t bus, uint8_t device_num,
                                        uint16_t vendor_id, uint16_t device_id,
                                        uint64_t reg_base, uint64_t reg_size) {
    return (device_t){
        .compatible = NULL,
        .name = NULL,
        .reg_base = reg_base,
        .reg_size = reg_size,
        .vendor_id = vendor_id,
        .device_id = device_id,
        .bus = bus,
        .device_num = device_num,
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
}

static void pci_process_device(uint8_t bus, uint8_t device_num,
                                device_callback_t callback, void* context,
                                int* device_count) {
    uint16_t vendor_id = pcie_ecam_read16(bus, device_num, 0, PCI_VENDOR_ID_OFFSET);

    if (!pci_vendor_is_valid(vendor_id)) {
        return;
    }

    uint16_t device_id = pcie_ecam_read16(bus, device_num, 0, PCI_DEVICE_ID_OFFSET);

    pci_print_device_header(vendor_id, device_id, bus, device_num);

    pci_bar_info_t bar0 = pci_read_bar(bus, device_num, 0);
    pci_bar_info_t bar1 = pci_read_bar(bus, device_num, 1);
    pci_bar_info_t bar4 = pci_read_bar(bus, device_num, 4);

    // Assign BARs if not already configured
    if (bar0.bar_size > 0) {
        bar0.bar_value = pci_assign_bar_if_needed(bus, device_num, 0, PCI_BAR0_OFFSET, bar0.bar_size);
    }
    if (bar1.bar_size > 0) {
        bar1.bar_value = pci_assign_bar_if_needed(bus, device_num, 0, PCI_BAR1_OFFSET, bar1.bar_size);
    }
    if (bar4.bar_size > 0) {
        bar4.bar_value = pci_assign_bar_if_needed(bus, device_num, 0, PCI_BAR4_OFFSET, bar4.bar_size);
    }

    pci_print_all_bars(bus, device_num);

    pci_selected_bar_t selected_bar = pci_select_bar_for_device(bus, device_num, vendor_id, device_id, &bar0, &bar1, &bar4);

    // Enable memory access in PCI command register
    uint16_t command = pcie_ecam_read16(bus, device_num, 0, PCI_COMMAND_OFFSET);
    command |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    pcie_ecam_write16(bus, device_num, 0, PCI_COMMAND_OFFSET, command);

    device_t device_info = pci_create_device_info(bus, device_num, vendor_id, device_id,
                                                   selected_bar.base_address, selected_bar.size);

    if (callback != NULL) {
        callback(&device_info, context);
    }

    (*device_count)++;
}

static int pcie_enumerate_devices(device_callback_t callback, void* context) {
    int device_count = 0;

    for (uint8_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
        for (uint8_t device_num = 0; device_num < PCI_MAX_DEVICES; device_num++) {
            pci_process_device(bus, device_num, callback, context, &device_count);
        }
    }

    return device_count;
}

// Track MMIO device numbers for sequential assignment
static uint8_t next_mmio_device_num = 1;

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
        // Assign sequential device number for MMIO devices
        // Use bus=0 for MMIO devices, device_num increments
        mutable_device.bus = 0;
        mutable_device.device_num = next_mmio_device_num++;

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
