#pragma once

#include "types.h"

// Forward declaration
typedef struct device device_t;

// Driver types
typedef enum {
    DRIVER_TYPE_NETWORK,
    DRIVER_TYPE_STORAGE,
    DRIVER_TYPE_DISPLAY,
    DRIVER_TYPE_INPUT,
    DRIVER_TYPE_RANDOM,
    DRIVER_TYPE_UNKNOWN
} driver_type_t;

// Device ID matching table entry
typedef struct {
    const char *compatible;     // Device tree compatible string
    uint16_t vendor_id;         // PCI vendor ID (optional, for identification)
    uint16_t device_id;         // PCI device ID (optional, for identification)
    const char *name;           // Human-readable device name
} device_id_t;

// Driver descriptor
typedef struct driver {
    const char *name;
    const char *version;
    driver_type_t type;
    const device_id_t *id_table;

    // Per-instance operations (called by resource manager)
    int (*init_context)(void *ctx, device_t *device);
    void (*deinit_context)(void *ctx);
} driver_t;
