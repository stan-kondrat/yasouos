#pragma once

#include "../../common/types.h"

/**
 * Initialize hardware RNG (optional)
 * Acquires exclusive access to hardware RNG device via resource manager.
 */
void random_hardware_init(void);

/**
 * Get random bytes
 * Uses hardware RNG if available, otherwise software PRNG (xorshift64).
 * Software fallback always available, no init required.
 *
 * @return Number of bytes generated, or -1 on failure
 */
int random_get_bytes(uint8_t *buffer, size_t length);
