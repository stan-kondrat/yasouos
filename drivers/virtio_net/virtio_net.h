#pragma once

#include "../../common/types.h"
#include "../../common/drivers.h"
#include "../../kernel/devices/devices.h"

/**
 * Get virtio-net driver descriptor
 * @return Pointer to driver descriptor
 */
const driver_t* virtio_net_get_driver(void);
