#include "devices.h"
#include "../../common/common.h"

int devices_scan(void) {
    puts("Scanning device tree...\n");
    int device_count = devices_enumerate(NULL, NULL);
    puts("Found ");
    put_hex16(device_count);
    puts(" device(s)\n\n");
    return device_count;
}
