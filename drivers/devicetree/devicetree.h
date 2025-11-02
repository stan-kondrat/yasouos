#pragma once

#include "../../include/types.h"

// Device information from device tree
typedef struct {
    const char *compatible;      // Device compatible string
    uint64_t reg_base;          // Register base address
    uint64_t reg_size;          // Register region size
    uint16_t vendor_id;         // PCI vendor ID (if applicable)
    uint16_t device_id;         // PCI device ID (if applicable)
    uint8_t bus;                // PCI bus number (if applicable)
    uint8_t device;             // PCI device number (if applicable)
    uint8_t function;           // PCI function number (if applicable)
} dt_device_t;

// Device enumeration callback
typedef void (*dt_device_callback_t)(const dt_device_t *device, void *context);

/**
 * Initialize device tree subsystem
 * @return 0 on success, -1 on failure
 */
int dt_init(void);

/**
 * Enumerate all devices in the device tree
 * @param callback Function to call for each device
 * @param context User-provided context
 * @return Number of devices found
 */
int dt_enumerate_devices(dt_device_callback_t callback, void *context);

/**
 * Find device by compatible string
 * @param compatible Compatible string to search for
 * @param device Output device information
 * @return 1 if found, 0 if not found
 */
int dt_find_device(const char *compatible, dt_device_t *device);

/**
 * Get device name from vendor/device IDs
 * @param vendor_id Vendor ID
 * @param device_id Device ID
 * @return Device name or NULL if not found
 */
const char *dt_get_device_name(uint16_t vendor_id, uint16_t device_id);

/**
 * Scan and count all devices in the device tree (with output)
 * @return Number of devices found
 */
int devicetree_scan(void);
