#include "devices.h"
#include "../drivers/drivers.h"
#include "../../common/common.h"

// Device registry (linked list)
static device_t *device_list_head = NULL;
static device_t *device_tree_root = NULL;
static uintptr_t fdt_address = 0;

// Callback to add device to registry
static void add_device_to_registry(const device_t *device, [[maybe_unused]] void *context) {
    // Allocate static storage for devices (simple fixed-size array for now)
    static device_t device_storage[128];
    static int device_count = 0;

    if (device_count >= 128) {
        return;  // Registry full
    }

    // Copy device info field by field (explicit assignment works better with -O3)
    device_storage[device_count].compatible = device->compatible;
    device_storage[device_count].name = device->name;
    device_storage[device_count].reg_base = device->reg_base;
    device_storage[device_count].reg_size = device->reg_size;
    device_storage[device_count].vendor_id = device->vendor_id;
    device_storage[device_count].device_id = device->device_id;
    device_storage[device_count].bus = device->bus;
    device_storage[device_count].device_num = device->device_num;
    device_storage[device_count].function = device->function;
    device_storage[device_count].state = DEVICE_STATE_DISCOVERED;
    device_storage[device_count].driver = NULL;
    device_storage[device_count].mmio_virt = NULL;
    device_storage[device_count].parent = NULL;
    device_storage[device_count].first_child = NULL;
    device_storage[device_count].next_sibling = NULL;
    device_storage[device_count].depth = device->depth;
    device_storage[device_count].next = NULL;

    device_t *new_device = &device_storage[device_count];

    // Add to linked list (tree hierarchy will be built later)
    if (!device_list_head) {
        device_list_head = new_device;
    } else {
        device_t *current = device_list_head;
        while (current->next) {
            current = current->next;
        }
        current->next = new_device;
    }

    device_count++;
}

// Build tree hierarchy after all devices are added
static void build_tree_hierarchy(void) {
    // Simple approach: assume all non-root devices are children of the first root
    // This works for simple device trees like QEMU virt machines
    device_t *root = NULL;

    // Find first root device
    device_t *current = device_list_head;
    while (current) {
        if (current->depth == 0 && !root) {
            root = current;
            device_tree_root = root;
            break;
        }
        current = current->next;
    }

    // Link all depth-1 devices as children of root
    if (root) {
        current = device_list_head;
        device_t *last_child = NULL;

        while (current) {
            if (current->depth == 1 && current != root) {
                current->parent = root;

                if (!root->first_child) {
                    root->first_child = current;
                    last_child = current;
                } else if (last_child) {
                    last_child->next_sibling = current;
                    last_child = current;
                }
            }
            current = current->next;
        }
    }
}

int devices_scan(void) {
    puts("Scanning device tree...\n");
    int device_count = devices_enumerate(add_device_to_registry, NULL);
    puts("Found ");
    put_hex16(device_count);
    puts(" device(s)\n");

    // Build tree hierarchy from flat list
    build_tree_hierarchy();

    return device_count;
}

device_t* devices_get_first(void) {
    return device_list_head;
}

device_t* devices_get_next(device_t *current) {
    if (!current) {
        return NULL;
    }
    return current->next;
}

device_driver_t* device_get_driver(device_t *device) {
    if (!device) {
        return NULL;
    }
    return device->driver;
}

void device_set_driver(device_t *device, device_driver_t *driver) {
    if (device) {
        device->driver = driver;
    }
}

void* device_map_mmio(device_t *device) {
    if (!device) {
        return NULL;
    }

    // For now, use identity mapping (physical == virtual)
    // In a real OS with virtual memory, this would map the physical address
    device->mmio_virt = (void*)(uintptr_t)device->reg_base;
    return device->mmio_virt;
}

void device_unmap_mmio(device_t *device) {
    if (device) {
        device->mmio_virt = NULL;
    }
}

device_t* device_tree_get_root(void) {
    return device_tree_root;
}

device_t* device_get_parent(device_t *device) {
    if (!device) {
        return NULL;
    }
    return device->parent;
}

device_t* device_get_first_child(device_t *device) {
    if (!device) {
        return NULL;
    }
    return device->first_child;
}

device_t* device_get_next_sibling(device_t *device) {
    if (!device) {
        return NULL;
    }
    return device->next_sibling;
}

device_t* device_find_by_name(const char *name) {
    if (!name) {
        return NULL;
    }

    device_t *current = device_list_head;
    while (current) {
        if (current->name && strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

// Helper to print device info with indentation
static void print_device(const device_t *device, int indent) {
    // Indentation with "  - " prefix style
    puts("  ");
    for (int i = 0; i < indent; i++) {
        puts("  ");
    }
    puts("- ");

    // Device name or compatible string
    if (device->name) {
        puts(device->name);
    } else if (device->compatible) {
        puts(device->compatible);
    } else {
        puts("(unnamed)");
    }

    // Address
    puts(" @ 0x");
    put_hex64(device->reg_base);

    // Optional: vendor/device IDs for PCI/VirtIO
    if (device->vendor_id || device->device_id) {
        puts(" [");
        put_hex16(device->vendor_id);
        puts(":");
        put_hex16(device->device_id);
        puts("]");
    }

    // Optional: driver binding
    if (device->driver) {
        puts(" -> ");
        puts(device->driver->name);
    }

    puts("\n");

    // Print children recursively
    device_t *child = device->first_child;
    while (child) {
        print_device(child, indent + 1);
        child = child->next_sibling;
    }
}

void device_tree_print(void) {
    puts("Device tree:\n");

    if (device_tree_root) {
        // Print tree hierarchy starting from all root-level devices
        device_t *current = device_list_head;
        while (current) {
            // Only print devices without a parent (root-level)
            if (!current->parent) {
                print_device(current, 0);
            }
            current = current->next;
        }
    } else {
        // Fallback: print flat list (no hierarchy)
        device_t *current = device_list_head;
        while (current) {
            print_device(current, 0);
            current = current->next;
        }
    }
}

void device_set_fdt(uintptr_t fdt_addr) {
    fdt_address = fdt_addr;
}

uintptr_t device_get_fdt(void) {
    return fdt_address;
}
