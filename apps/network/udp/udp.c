#include "udp.h"
#include "../net_utils.h"
#include "../ipv4/ipv4.h"

void udp_print(const uint8_t *ip_packet, size_t length, int leftpad) {
    if (!ip_packet || length < sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t)) {
        puts("  [UDP] Packet too small\n");
        return;
    }

    // Extract IPv4 header to find UDP offset
    const ipv4_hdr_t *ip = (const ipv4_hdr_t *)ip_packet;
    uint8_t ihl = (ip->version_ihl & 0x0F) * 4;

    if (length < ihl + sizeof(udp_hdr_t)) {
        puts("  [UDP] Packet too small for UDP header\n");
        return;
    }

    // UDP header is after IPv4 header
    const udp_hdr_t *udp = (const udp_hdr_t *)(ip_packet + ihl);
    uint16_t src_port = ntohs_unaligned(&udp->src_port);
    uint16_t dst_port = ntohs_unaligned(&udp->dst_port);
    uint16_t len = ntohs_unaligned(&udp->length);

    for (int i = 0; i < leftpad; i++) {
        putchar(' ');
    }
    puts("[UDP] ");
    net_print_decimal_u16(src_port);
    puts(" -> ");
    net_print_decimal_u16(dst_port);
    puts(" len=");
    net_print_decimal_u16(len);
    puts("\n");

    // Print hexdump of entire IP packet starting from IPv4 header
    size_t offset = 0;
    while (offset < length) {
        // Print leftpad + extra indent for hex lines
        for (int i = 0; i < leftpad + 4; i++) {
            putchar(' ');
        }

        // Print offset
        puts("0x");
        put_hex16(offset);
        puts(":  ");

        // Print hex bytes (16 per line)
        size_t line_len = (length - offset > 16) ? 16 : (length - offset);
        for (size_t i = 0; i < 16; i++) {
            if (i < line_len) {
                put_hex8(ip_packet[offset + i]);
            } else {
                puts("  ");
            }
            putchar(' ');
            if (i == 7) {
                putchar(' ');
            }
        }

        // Print ASCII representation
        puts(" ");
        for (size_t i = 0; i < line_len; i++) {
            uint8_t c = ip_packet[offset + i];
            if (c >= 32 && c <= 126) {
                putchar(c);
            } else {
                putchar('.');
            }
        }

        puts("\n");
        offset += 16;
    }
}

void udp_build_header(udp_hdr_t *header, uint16_t src_port, uint16_t dst_port, uint16_t payload_length) {
    header->src_port = htons(src_port);
    header->dst_port = htons(dst_port);
    header->length = htons(sizeof(udp_hdr_t) + payload_length);
    // UDP checksum is optional for IPv4 (RFC 768)
    // Setting to 0 indicates no checksum calculation
    header->checksum = 0;
}
