#include "../common/common.h"
#include "drivers/drivers.h"
#include "../drivers/virtio_net/virtio_net.h"
#include "../drivers/virtio_rng/virtio_rng.h"
#include "../drivers/virtio_blk/virtio_blk.h"

void drivers_config_register(void) {
    puts("Registering drivers...\n");

    virtio_net_register_driver();
    virtio_rng_register_driver();
    virtio_blk_register_driver();

    puts("\n");
}
