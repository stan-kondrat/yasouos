#pragma once

#include "../../common/types.h"

// Forward declarations
typedef struct device device_t;
typedef struct device_driver device_driver_t;

// Device lifecycle states
typedef enum {
    DEVICE_STATE_DISCOVERED,    // Found during enumeration, not yet bound
    DEVICE_STATE_BOUND,         // Driver matched and probe called
    DEVICE_STATE_ACTIVE,        // Device initialized and operational
    DEVICE_STATE_REMOVED        // Device removed or failed
} device_state_t;

// Device information from device tree
struct device {
    const char *compatible;      // Device compatible string
    const char *name;           // Device node name (from device tree)
    uint64_t reg_base;          // Register base address
    uint64_t reg_size;          // Register region size
    uint16_t vendor_id;         // PCI vendor ID (if applicable)
    uint16_t device_id;         // PCI device ID (if applicable)
    uint8_t bus;                // PCI bus number (if applicable)
    uint8_t device_num;         // PCI device number (if applicable)
    uint8_t function;           // PCI function number (if applicable)

    device_driver_t *driver;    // Associated driver (set during probe)
    device_state_t state;       // Current device state
    void *mmio_virt;            // Virtual address of mapped MMIO region

    // Device tree hierarchy
    device_t *parent;           // Parent device in tree
    device_t *first_child;      // First child device
    device_t *next_sibling;     // Next sibling in parent's child list
    int depth;                  // Tree depth (used during enumeration only)

    device_t *next;             // Linked list for flat iteration
};

// Device enumeration callback
typedef void (*device_callback_t)(const device_t *device, void *context);

/**
 * Enumerate all devices in the device tree
 * @param callback Function to call for each device
 * @param context User-provided context
 * @return Number of devices found
 */
int devices_enumerate(device_callback_t callback, void *context);

/**
 * Find device by compatible string
 * @param compatible Compatible string to search for
 * @param device Output device information
 * @return 1 if found, 0 if not found
 */
int devices_find(const char *compatible, device_t *device);

/**
 * Get device name from vendor/device IDs
 * @param vendor_id Vendor ID
 * @param device_id Device ID
 * @return Device name or NULL if not found
 */
const char *devices_get_name(uint16_t vendor_id, uint16_t device_id);

/**
 * Scan and count all devices in the device tree (with output)
 * @return Number of devices found
 */
int devices_scan(void);

/**
 * Get first device in the device list
 * @return Pointer to first device, or NULL if no devices
 */
device_t* devices_get_first(void);

/**
 * Get next device in the device list
 * @param current Current device
 * @return Pointer to next device, or NULL if no more devices
 */
device_t* devices_get_next(device_t *current);

/**
 * Get driver associated with a device
 * @param device Device to query
 * @return Pointer to driver, or NULL if no driver bound
 */
device_driver_t* device_get_driver(device_t *device);

/**
 * Set driver for a device (called during probe)
 * @param device Device to update
 * @param driver Driver to associate
 */
void device_set_driver(device_t *device, device_driver_t *driver);

/**
 * Map device MMIO region to virtual address space
 * @param device Device to map
 * @return Virtual address of mapped region, or NULL on failure
 */
void* device_map_mmio(device_t *device);

/**
 * Unmap device MMIO region
 * @param device Device to unmap
 */
void device_unmap_mmio(device_t *device);

/**
 * Get root device of the device tree
 * @return Pointer to root device, or NULL if tree not initialized
 */
device_t* device_tree_get_root(void);

/**
 * Get parent of a device in the tree
 * @param device Device to query
 * @return Pointer to parent device, or NULL if root or orphan
 */
device_t* device_get_parent(device_t *device);

/**
 * Get first child of a device in the tree
 * @param device Parent device
 * @return Pointer to first child, or NULL if no children
 */
device_t* device_get_first_child(device_t *device);

/**
 * Get next sibling of a device in the tree
 * @param device Current device
 * @return Pointer to next sibling, or NULL if last sibling
 */
device_t* device_get_next_sibling(device_t *device);

/**
 * Find device by name in the tree
 * @param name Device name to search for
 * @return Pointer to device, or NULL if not found
 */
device_t* device_find_by_name(const char *name);

/**
 * Print complete device tree to console
 * Shows all devices in a hierarchical format
 */
void device_tree_print(void);

/**
 * Set FDT address for device tree parsing
 * @param fdt_addr Physical address of FDT blob
 */
void device_set_fdt(uintptr_t fdt_addr);

/**
 * Get stored FDT address
 * @return Physical address of FDT blob, or 0 if not set
 */
uintptr_t device_get_fdt(void);
