#include "devicetree.h"
#include "../../include/common.h"

int devicetree_scan(void) {
    puts("Scanning device tree...\n");
    dt_init();
    int device_count = dt_enumerate_devices(NULL, NULL);
    puts("Found ");
    put_hex16(device_count);
    puts(" device(s)\n\n");
    return device_count;
}
