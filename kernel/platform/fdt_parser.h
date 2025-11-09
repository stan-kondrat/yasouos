#pragma once

#include "../../common/types.h"
#include "../devices/devices.h"

// ============================================================================
// Flattened Device Tree (FDT) Parser
// ============================================================================
//
// FDT Specification: https://devicetree-specification.readthedocs.io/
//
// ============================================================================

/**
 * Extract kernel command line from device tree /chosen/bootargs
 *
 * @param fdt_addr Physical address of the device tree blob
 * @return Pointer to bootargs string, or NULL if not found
 */
const char* fdt_get_bootargs(uintptr_t fdt_addr);

/**
 * Parse device tree and enumerate all devices
 *
 * @param fdt_addr Physical address of the device tree blob
 * @param callback Function to call for each device found
 * @param context User context passed to callback
 * @return Number of devices enumerated, or -1 on error
 */
int fdt_enumerate_devices(uintptr_t fdt_addr, device_callback_t callback, void *context);
