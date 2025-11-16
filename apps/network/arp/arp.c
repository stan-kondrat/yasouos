#include "arp.h"
#include "../net_utils.h"
#include "../ethernet/ethernet.h"
#include "../../../common/common.h"

void arp_print(const arp_hdr_t *arp, int leftpad) {
    if (!arp) {
        return;
    }

    uint16_t opcode = ntohs(arp->opcode);

    for (int i = 0; i < leftpad; i++) {
        putchar(' ');
    }
    if (opcode == ARP_OP_REQUEST) {
        puts("ARP Request: who-has ");
        net_print_ip(ntohl(arp->target_ip));
        puts(" tell ");
        net_print_ip(ntohl(arp->sender_ip));
        puts(" (");
        net_print_mac(arp->sender_mac);
        puts(")\n");
    } else if (opcode == ARP_OP_REPLY) {
        puts("ARP Reply: ");
        net_print_ip(ntohl(arp->sender_ip));
        puts(" is-at ");
        net_print_mac(arp->sender_mac);
        puts("\n");
    } else {
        puts("ARP opcode=");
        put_hex16(opcode);
        puts("\n");
    }
}

void arp_build_request(uint8_t *packet,
                       const uint8_t sender_mac[6],
                       uint32_t sender_ip,
                       uint32_t target_ip) {
    eth_hdr_t *eth = (eth_hdr_t *)packet;
    arp_hdr_t *arp = (arp_hdr_t *)(packet + sizeof(eth_hdr_t));

    uint8_t broadcast[6] = BROADCAST_MAC;
    for (int i = 0; i < 6; i++) {
        eth->dst[i] = broadcast[i];
        eth->src[i] = sender_mac[i];
    }
    eth->type = htons(ETH_P_ARP);

    arp->hw_type = htons(ARP_HW_ETHER);
    arp->proto_type = htons(ETH_P_IP);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(ARP_OP_REQUEST);

    for (int i = 0; i < 6; i++) {
        arp->sender_mac[i] = sender_mac[i];
        arp->target_mac[i] = 0x00;
    }
    arp->sender_ip = htonl(sender_ip);
    arp->target_ip = htonl(target_ip);
}

void arp_build_reply(uint8_t *packet,
                     const uint8_t sender_mac[6],
                     uint32_t sender_ip,
                     const uint8_t target_mac[6],
                     uint32_t target_ip) {
    eth_hdr_t *eth = (eth_hdr_t *)packet;
    arp_hdr_t *arp = (arp_hdr_t *)(packet + sizeof(eth_hdr_t));

    for (int i = 0; i < 6; i++) {
        eth->dst[i] = target_mac[i];
        eth->src[i] = sender_mac[i];
    }
    eth->type = htons(ETH_P_ARP);

    arp->hw_type = htons(ARP_HW_ETHER);
    arp->proto_type = htons(ETH_P_IP);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(ARP_OP_REPLY);

    for (int i = 0; i < 6; i++) {
        arp->sender_mac[i] = sender_mac[i];
        arp->target_mac[i] = target_mac[i];
    }
    arp->sender_ip = htonl(sender_ip);
    arp->target_ip = htonl(target_ip);
}

const arp_hdr_t* arp_parse(const uint8_t *packet, size_t length, arp_hdr_t *out_arp) {
    if (length < ARP_PACKET_SIZE) {
        return NULL;
    }

    const arp_hdr_t *arp = (const arp_hdr_t *)(packet + sizeof(eth_hdr_t));

    if (out_arp) {
        *out_arp = *arp;
    }

    return arp;
}
