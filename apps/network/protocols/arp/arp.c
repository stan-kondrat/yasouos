#include "arp.h"
#include "../../../../common/common.h"

static inline uint16_t ntohs(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint32_t ntohl(uint32_t x) {
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
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

static void print_ip(uint32_t ip) {
    print_decimal((ip >> 24) & 0xFF);
    puts(".");
    print_decimal((ip >> 16) & 0xFF);
    puts(".");
    print_decimal((ip >> 8) & 0xFF);
    puts(".");
    print_decimal(ip & 0xFF);
}

void arp_print(const arp_hdr_t *arp) {
    if (!arp) {
        return;
    }

    uint16_t opcode = ntohs(arp->opcode);

    if (opcode == ARP_OP_REQUEST) {
        puts("  [ARP] Request: who-has ");
        print_ip(ntohl(arp->target_ip));
        puts(" tell ");
        print_ip(ntohl(arp->sender_ip));
        puts(" (");
        print_mac(arp->sender_mac);
        puts(")\n");
    } else if (opcode == ARP_OP_REPLY) {
        puts("  [ARP] Reply: ");
        print_ip(ntohl(arp->sender_ip));
        puts(" is-at ");
        print_mac(arp->sender_mac);
        puts("\n");
    } else {
        puts("  [ARP] opcode=");
        put_hex16(opcode);
        puts("\n");
    }
}
