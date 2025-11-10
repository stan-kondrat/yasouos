#pragma once

#include "../devices/devices.h"
#include "../../common/drivers.h"

/**
 * Resource handle returned by resource manager
 */
typedef struct resource resource_t;

/**
 * Set devices list for resource manager
 * Call after devices_scan() during kernel boot
 *
 * @param devices Linked list of devices from devices_scan()
 */
void resources_set_devices(device_t *devices);

/**
 * Update devices list for resource manager (hotplug support)
 * For future hotplug/dynamic device discovery
 *
 * @param devices Updated linked list of devices
 */
void resources_update_devices(device_t *devices);

/**
 * Acquire first available device with specified driver
 * Device is acquired exclusively
 *
 * @param driver Driver descriptor pointer
 * @return Resource pointer on success, NULL if unavailable
 */
resource_t* resource_acquire_available(const driver_t *driver, void *context);

/**
 * Release device resource
 *
 * @param resource Resource to release
 * @return 0 on success, -1 on error
 */
int resource_release(resource_t *resource);

/**
 * Get device from resource
 *
 * @param resource Resource handle
 * @return Device pointer, or NULL if invalid resource
 */
device_t* resource_get_device(resource_t *resource);
