#include "ipv4.h"
#include "../tcp/tcp.h"
#include "../udp/udp.h"
#include "../net_utils.h"

void ipv4_print(const uint8_t *packet, size_t length, int leftpad) {
    if (!packet || length < sizeof(ipv4_hdr_t)) {
        puts("[IPv4] Packet too small\n");
        return;
    }

    const ipv4_hdr_t *ip = (const ipv4_hdr_t *)packet;
    uint16_t total_len = ntohs(ip->total_length);
    uint32_t src_ip = ntohl(ip->src_ip);
    uint32_t dst_ip = ntohl(ip->dst_ip);

    for (int i = 0; i < leftpad; i++) {
        putchar(' ');
    }
    puts("[IPv4] ");
    net_print_ip(src_ip);
    puts(" -> ");
    net_print_ip(dst_ip);
    puts(" proto=");
    net_print_decimal_u8(ip->protocol);
    puts(" ttl=");
    net_print_decimal_u8(ip->ttl);
    puts(" len=");
    net_print_decimal_u16(total_len);
    puts("\n");

    uint8_t ihl = (ip->version_ihl & 0x0F) * 4;
    if (length < ihl) {
        return;
    }

    const uint8_t *payload = packet + ihl;
    size_t payload_len = (length > total_len) ? (size_t)(total_len - ihl) : (length - ihl);

    if (ip->protocol == IPPROTO_TCP) {
        tcp_print(payload, payload_len, leftpad + 2);
    } else if (ip->protocol == IPPROTO_UDP) {
        udp_print(packet, length, leftpad + 2);
    }
}

uint16_t ipv4_checksum(const ipv4_hdr_t *header) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)header;

    for (size_t i = 0; i < sizeof(ipv4_hdr_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return htons(~sum);
}

bool ipv4_verify_checksum(const ipv4_hdr_t *header) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)header;

    for (size_t i = 0; i < sizeof(ipv4_hdr_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (sum & 0xFFFF) == 0xFFFF;
}

void ipv4_build_header(ipv4_hdr_t *header, uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, uint16_t payload_length, uint8_t ttl) {
    header->version_ihl = 0x45;
    header->tos = 0;
    header->total_length = htons(sizeof(ipv4_hdr_t) + payload_length);
    header->identification = htons(1);
    header->flags_fragment = 0;
    header->ttl = ttl;
    header->protocol = protocol;
    header->checksum = 0;
    header->src_ip = src_ip;
    header->dst_ip = dst_ip;

    header->checksum = ipv4_checksum(header);
}
