#include "../include/common.h"
#include "driver_registry.h"
#include "virtio_net/virtio_net.h"
#include "virtio_rng/virtio_rng.h"
#include "virtio_blk/virtio_blk.h"

void register_drivers(void) {
    puts("Registering drivers...\n");

    virtio_net_register_driver();
    virtio_rng_register_driver();
    virtio_blk_register_driver();

    puts("\n");
}

void probe_devices(void) {
    puts("Probing devices...\n");
    int probed_count = driver_probe_all();

    if (probed_count > 0) {
        puts("\nSuccessfully initialized ");
        put_hex16(probed_count);
        puts(" device(s)\n");
    }
}
