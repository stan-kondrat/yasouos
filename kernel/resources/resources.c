#include "resources.h"
#include "../../common/common.h"

#define MAX_RESOURCE_ALLOCATIONS 16

// Resource handle (exposed as opaque type in header)
struct resource {
    device_t *device;
    const driver_t *driver;
    void *context;
    struct resource *next;
    bool in_use;
};

// Global state
static struct {
    device_t *devices;           // Device list from devices_scan()
    resource_t *allocations;     // Allocated resources
    resource_t pool[MAX_RESOURCE_ALLOCATIONS];  // Static resource pool
} resources = {
    .devices = NULL,
    .allocations = NULL,
    .pool = {{0}}
};

/**
 * Check if device matches driver's ID table
 */
static bool device_matches_driver(const device_t *device, const driver_t *driver) {
    if (!driver->id_table) {
        return false;
    }

    const device_id_t *id_entry = driver->id_table;
    while (id_entry->compatible || id_entry->vendor_id || id_entry->device_id) {
        // Check compatible string if present
        if (id_entry->compatible && device->compatible) {
            if (strcmp(id_entry->compatible, device->compatible) == 0) {
                return true;
            }
        }

        // Check vendor/device IDs if present
        if (id_entry->vendor_id && id_entry->device_id) {
            if (device->vendor_id == id_entry->vendor_id &&
                device->device_id == id_entry->device_id) {
                return true;
            }
        }

        id_entry++;
    }

    return false;
}

/**
 * Check if device is already allocated
 */
static bool device_is_allocated(const device_t *device) {
    for (resource_t *r = resources.allocations; r != NULL; r = r->next) {
        if (r->device == device) {
            return true;
        }
    }
    return false;
}

/**
 * Allocate a resource from the static pool
 */
static resource_t* resource_alloc(void) {
    for (int i = 0; i < MAX_RESOURCE_ALLOCATIONS; i++) {
        if (!resources.pool[i].in_use) {
            resources.pool[i].in_use = true;
            return &resources.pool[i];
        }
    }
    return NULL;
}

/**
 * Free a resource back to the pool
 */
static void resource_free(resource_t *resource) {
    resource->in_use = false;
    resource->device = NULL;
    resource->driver = NULL;
    resource->context = NULL;
    resource->next = NULL;
}

/**
 * Add resource to allocations list
 */
static void resource_add_allocation(resource_t *resource) {
    resource->next = resources.allocations;
    resources.allocations = resource;
}

/**
 * Remove resource from allocations list
 */
static bool resource_remove_allocation(resource_t *resource) {
    if (resources.allocations == resource) {
        resources.allocations = resource->next;
        return true;
    }

    for (resource_t *r = resources.allocations; r != NULL; r = r->next) {
        if (r->next == resource) {
            r->next = resource->next;
            return true;
        }
    }

    return false;
}

void resources_set_devices(device_t *devices) {
    resources.devices = devices;
}

void resources_update_devices(device_t *devices) {
    (void)devices;
    puts("resources_update_devices: unimplemented\n");
}

resource_t* resource_acquire_available(const driver_t *driver, void *context) {
    if (!driver || !context) {
        return NULL;
    }

    // Iterate through device list
    for (device_t *device = resources.devices; device != NULL; device = device->next) {
        // Check if device matches driver
        if (!device_matches_driver(device, driver)) {
            continue;
        }

        // Check if device is already allocated
        if (device_is_allocated(device)) {
            continue;
        }

        // Allocate resource from pool
        resource_t *resource = resource_alloc();
        if (!resource) {
            // No free resources in pool
            return NULL;
        }

        // Initialize resource
        resource->device = device;
        resource->driver = driver;
        resource->context = context;

        // Call driver init_context if provided
        if (driver->init_context) {
            int result = driver->init_context(context, device);
            if (result != 0) {
                // Init failed, free resource and try next device
                resource_free(resource);
                continue;
            }
        }

        // Add to allocations list
        resource_add_allocation(resource);

        return resource;
    }

    return NULL;  // No matching device found
}

int resource_release(resource_t *resource) {
    if (!resource) {
        return -1;
    }

    // Remove from allocations list
    if (!resource_remove_allocation(resource)) {
        return -1;  // Resource not in allocations list
    }

    // Call driver deinit_context if provided
    if (resource->driver && resource->driver->deinit_context && resource->context) {
        resource->driver->deinit_context(resource->context);
    }

    // Free resource back to pool
    resource_free(resource);

    return 0;
}

device_t* resource_get_device(resource_t *resource) {
    if (!resource) {
        return NULL;
    }
    return resource->device;
}

void resource_print_tag(const resource_t *resource) {
    if (!resource || !resource->driver || !resource->device) {
        return;
    }

    const driver_t *driver = resource->driver;
    const device_t *device = resource->device;

    puts("[");
    put_hex8(device->bus);
    puts(":");
    put_hex8(device->device_num);
    puts("|");
    puts(driver->name);
    puts("@");
    puts(driver->version);
    puts("]");
}
