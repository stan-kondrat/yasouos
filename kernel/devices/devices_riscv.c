#include "devices.h"
#include "virtio_mmio.h"
#include "../platform/fdt_parser.h"
#include "../../common/common.h"

// Callback wrapper to probe VirtIO devices from FDT
static void probe_virtio_callback(const device_t *device, void *context) {
    // Only process virtio,mmio devices
    if (!device->compatible || strcmp(device->compatible, "virtio,mmio") != 0) {
        // Pass through non-VirtIO devices directly
        device_callback_t user_callback = (device_callback_t)context;
        if (user_callback) {
            user_callback(device, NULL);
        }
        return;
    }

    // Create a mutable copy to probe
    device_t mutable_device = *device;

    // Probe the VirtIO device to get vendor/device IDs
    if (virtio_mmio_probe_device(&mutable_device) == 0) {
        // Successfully probed - pass to user callback
        device_callback_t user_callback = (device_callback_t)context;
        if (user_callback) {
            user_callback(&mutable_device, NULL);
        }
    }
}

int devices_enumerate(device_callback_t callback, [[maybe_unused]] void *context) {
    uintptr_t fdt_addr = device_get_fdt();
    if (fdt_addr == 0) {
        return 0;
    }

    // Use FDT enumeration with VirtIO probing wrapper
    return fdt_enumerate_devices(fdt_addr, probe_virtio_callback, (void*)callback);
}

int devices_find([[maybe_unused]] const char *compatible,
                   [[maybe_unused]] device_t *device) {
    return 0;
}

const char *devices_get_name([[maybe_unused]] uint16_t vendor_id,
                               [[maybe_unused]] uint16_t device_id) {
    return NULL;
}
