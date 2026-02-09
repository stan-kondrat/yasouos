/*
 * Test Platform Header
 * Platform-specific functions for test kernels.
 * I/O functions (putchar, puts, put_hex*) are provided by common.h.
 */

#ifndef TEST_PLATFORM_H
#define TEST_PLATFORM_H

#include "../../common/common.h"

void platform_exit(int code);

#endif
