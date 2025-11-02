#pragma once

#include "../include/types.h"
#include "devicetree/devicetree.h"

// Driver types
typedef enum {
    DRIVER_TYPE_NETWORK,
    DRIVER_TYPE_STORAGE,
    DRIVER_TYPE_DISPLAY,
    DRIVER_TYPE_INPUT,
    DRIVER_TYPE_UNKNOWN
} driver_type_t;

// Driver registration result
typedef enum {
    DRIVER_REG_SUCCESS = 0,
    DRIVER_REG_ERROR_NULL,
    DRIVER_REG_ERROR_FULL
} driver_reg_result_t;

// Device ID matching table entry
typedef struct {
    const char *compatible;     // Device tree compatible string
    uint16_t vendor_id;         // PCI vendor ID (optional, for identification)
    uint16_t device_id;         // PCI device ID (optional, for identification)
    const char *name;           // Human-readable device name
} device_id_t;

// Driver operations
typedef struct driver_ops {
    int (*probe)(const dt_device_t *device);
    void (*remove)(const dt_device_t *device);
    const char *name;
} driver_ops_t;

// Driver descriptor
typedef struct device_driver {
    const char *name;
    const char *version;        // Driver version string
    driver_type_t type;
    const device_id_t *id_table;
    const driver_ops_t *ops;

    // Lifecycle hooks (for future dynamic loading)
    int (*init)(void);          // Called once when driver is registered
    void (*exit)(void);         // Called when driver is unregistered
} device_driver_t;

// Driver state (separate from descriptor to maintain const-correctness)
typedef struct driver_state {
    const device_driver_t *driver;
    int enabled;                // 1 = active, 0 = disabled
} driver_state_t;

/**
 * Register a device driver
 *
 * @param driver Driver descriptor to register
 * @return driver_reg_result_t status code
 */
driver_reg_result_t driver_register(const device_driver_t *driver);

/**
 * Find and probe all devices for registered drivers
 *
 * @return Number of devices successfully probed
 */
int driver_probe_all(void);

/**
 * Get driver information for a device
 *
 * @param vendor_id Vendor ID
 * @param device_id Device ID
 * @return Driver name or NULL if not found
 */
const char *driver_get_name(uint16_t vendor_id, uint16_t device_id);

/**
 * Unregister a device driver (for future dynamic loading)
 *
 * @param driver Driver descriptor to unregister
 * @return 0 on success, -1 on failure
 */
int driver_unregister(const device_driver_t *driver);

/**
 * Enable a registered driver by name and optional version
 *
 * @param name Driver name
 * @param version Driver version (NULL to match any version)
 * @return 0 on success, -1 on failure
 */
int driver_enable(const char *name, const char *version);

/**
 * Disable a registered driver by name and optional version
 *
 * @param name Driver name
 * @param version Driver version (NULL to match any version)
 * @return 0 on success, -1 on failure
 */
int driver_disable(const char *name, const char *version);
