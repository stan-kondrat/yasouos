#include "ethernet.h"
#include "../ipv4/ipv4.h"
#include "../arp/arp.h"
#include "../net_utils.h"
#include "../../../kernel/resources/resources.h"

void ethernet_print(const uint8_t *frame, size_t length, const void *resource, int leftpad) {
    if (!frame || length < sizeof(eth_hdr_t)) {
        puts("Ethernet frame too small\n");
        return;
    }

    const eth_hdr_t *eth = (const eth_hdr_t *)frame;
    uint16_t eth_type = ntohs(eth->type);

    if (resource) {
        resource_print_tag((const resource_t *)resource);
        puts(" ");
    }
    for (int i = 0; i < leftpad; i++) {
        putchar(' ');
    }
    puts("Ethernet ");
    net_print_mac(eth->src);
    puts(" -> ");
    net_print_mac(eth->dst);
    puts(" type=0x");
    put_hex16(eth_type);
    puts(" len=");
    net_print_decimal_u8(length);
    puts("\n");

    const uint8_t *payload = frame + sizeof(eth_hdr_t);
    size_t payload_len = length - sizeof(eth_hdr_t);

    if (eth_type == ETH_P_IP) {
        ipv4_print(payload, payload_len, leftpad + 2);
    } else if (eth_type == ETH_P_ARP) {
        arp_print((const arp_hdr_t *)payload, leftpad + 2);
    }
}
