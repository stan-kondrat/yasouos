#pragma once

#include "../../common/types.h"

// ============================================================================
// Minimal Flattened Device Tree (FDT) Parser
// ============================================================================
//
// This is a simplified FDT parser that can extract the bootargs string from
// the /chosen node. It implements just enough of the FDT spec to find and
// read the bootargs property.
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
