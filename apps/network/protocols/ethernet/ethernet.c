#include "ethernet.h"
#include "../../../../common/common.h"

static inline uint16_t ntohs(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static void print_mac(const uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) {
        put_hex8(mac[i]);
        if (i < 5) {
            puts(":");
        }
    }
}

static void print_decimal(uint8_t value) {
    if (value >= 100) {
        putchar('0' + (value / 100));
        value %= 100;
        putchar('0' + (value / 10));
        putchar('0' + (value % 10));
    } else if (value >= 10) {
        putchar('0' + (value / 10));
        putchar('0' + (value % 10));
    } else {
        putchar('0' + value);
    }
}

void ethernet_print(const uint8_t *frame, size_t length) {
    if (!frame || length < sizeof(eth_hdr_t)) {
        puts("[ETH] Frame too small\n");
        return;
    }

    const eth_hdr_t *eth = (const eth_hdr_t *)frame;
    uint16_t eth_type = ntohs(eth->type);

    puts("[ETH] ");
    print_mac(eth->src);
    puts(" -> ");
    print_mac(eth->dst);
    puts(" type=0x");
    put_hex16(eth_type);
    puts(" len=");
    print_decimal(length);
    puts("\n");
}
