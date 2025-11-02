#include "driver_registry.h"
#include "../include/common.h"
#include "devicetree/devicetree.h"

#define MAX_DRIVERS 16

static driver_state_t driver_states[MAX_DRIVERS];
static int driver_count = 0;

driver_reg_result_t driver_register(const device_driver_t *driver) {
    if (!driver) {
        puts("Error: Cannot register null driver\n");
        return DRIVER_REG_ERROR_NULL;
    }

    // Validate driver has name and version
    if (!driver->name || !driver->version) {
        puts("Error: Driver missing name or version\n");
        return DRIVER_REG_ERROR_NULL;
    }

    if (driver_count >= MAX_DRIVERS) {
        puts("Error: Driver registry full (max 16)\n");
        return DRIVER_REG_ERROR_FULL;
    }

    // Add to registry
    driver_states[driver_count].driver = driver;
    driver_states[driver_count].enabled = 1;
    driver_count++;

    // Print registration status
    puts("  - ");
    puts(driver->name);
    puts(" v");
    puts(driver->version);
    puts(" registered\n");

    return DRIVER_REG_SUCCESS;
}

static int device_matches_driver(const dt_device_t *device, const device_driver_t *driver) {
    const device_id_t *id = driver->id_table;

    while (id && (id->compatible || id->vendor_id != 0)) {
        // Match by compatible string if present
        if (id->compatible && device->compatible) {
            if (strcmp(id->compatible, device->compatible) == 0) {
                return 1;
            }
        }

        // Match by vendor/device ID
        if (id->vendor_id != 0 && id->vendor_id == device->vendor_id &&
            id->device_id == device->device_id) {
            return 1;
        }

        id++;
    }

    return 0;
}

static void probe_device_callback(const dt_device_t *device, void *context) {
    int *probe_count = (int *)context;

    for (int i = 0; i < driver_count; i++) {
        driver_state_t *state = &driver_states[i];
        const device_driver_t *driver = state->driver;

        // Skip disabled drivers
        if (!state->enabled) {
            continue;
        }

        if (device_matches_driver(device, driver)) {
            if (driver->ops && driver->ops->probe) {
                int result = driver->ops->probe(device);
                if (result == 0) {
                    (*probe_count)++;

                    // Print which device was initialized
                    puts("  - ");
                    puts(driver->name);
                    puts(" v");
                    puts(driver->version);
                    puts(" @ 0x");
                    put_hex32(device->reg_base);
                    puts(" [");
                    const device_id_t *id = driver->id_table;
                    while (id && (id->compatible || id->vendor_id != 0)) {
                        if ((id->compatible && device->compatible) ||
                            (id->vendor_id == device->vendor_id &&
                             id->device_id == device->device_id)) {
                            puts(id->name);
                            break;
                        }
                        id++;
                    }
                    puts("]\n");
                } else {
                    // Log probe failure
                    puts("  ! ");
                    puts(driver->name);
                    puts(" probe failed for device @ 0x");
                    put_hex32(device->reg_base);
                    puts(" (error ");
                    put_hex16(result);
                    puts(")\n");
                }
            }
            return;
        }
    }
}

int driver_probe_all(void) {
    int probe_count = 0;
    dt_enumerate_devices(probe_device_callback, &probe_count);
    return probe_count;
}

const char *driver_get_name(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < driver_count; i++) {
        const device_driver_t *driver = driver_states[i].driver;
        const device_id_t *id = driver->id_table;

        while (id && (id->compatible || id->vendor_id != 0)) {
            if (id->vendor_id == vendor_id && id->device_id == device_id) {
                return id->name;
            }
            id++;
        }
    }

    return 0;
}

static driver_state_t *driver_find_by_key(const char *name, const char *version) {
    if (!name) {
        return 0;
    }

    for (int i = 0; i < driver_count; i++) {
        const device_driver_t *driver = driver_states[i].driver;

        // Match by name
        if (!driver->name || strcmp(driver->name, name) != 0) {
            continue;
        }

        // If version specified, match by version too
        if (version) {
            if (!driver->version || strcmp(driver->version, version) != 0) {
                continue;
            }
        }

        return &driver_states[i];
    }

    return 0;
}

int driver_unregister(const device_driver_t *driver) {
    if (!driver) {
        return -1;
    }

    // Find and remove from registry
    for (int i = 0; i < driver_count; i++) {
        if (driver_states[i].driver == driver) {
            // Call exit hook if provided
            if (driver->exit) {
                driver->exit();
            }

            // Shift remaining drivers down
            for (int j = i; j < driver_count - 1; j++) {
                driver_states[j] = driver_states[j + 1];
            }
            driver_count--;
            return 0;
        }
    }

    return -1;  // Driver not found
}

int driver_enable(const char *name, const char *version) {
    driver_state_t *state = driver_find_by_key(name, version);
    if (!state) {
        return -1;
    }

    if (state->enabled) {
        return 0;  // Already enabled
    }

    state->enabled = 1;

    // TODO: Re-probe all devices to bind this driver
    // For now, just mark as enabled
    return 0;
}

int driver_disable(const char *name, const char *version) {
    driver_state_t *state = driver_find_by_key(name, version);
    if (!state) {
        return -1;
    }

    if (!state->enabled) {
        return 0;  // Already disabled
    }

    // TODO: Remove all devices using this driver
    // For now, just mark as disabled
    state->enabled = 0;

    return 0;
}
