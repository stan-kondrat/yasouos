#include "tcp.h"
#include "../net_utils.h"
#include "../ipv4/ipv4.h"

void tcp_print(const uint8_t *tcp_segment, size_t length, int leftpad) {
    if (!tcp_segment || length < sizeof(tcp_hdr_t)) {
        puts("  [TCP] Segment too small\n");
        return;
    }

    const tcp_hdr_t *tcp = (const tcp_hdr_t *)tcp_segment;
    uint16_t src_port = ntohs_unaligned(&tcp->src_port);
    uint16_t dst_port = ntohs_unaligned(&tcp->dst_port);
    uint32_t seq = ntohl_unaligned(&tcp->seq_num);
    uint32_t ack = ntohl_unaligned(&tcp->ack_num);
    uint16_t win = ntohs_unaligned(&tcp->window);
    uint8_t data_offset = (tcp->data_offset >> 4) * 4;

    for (int i = 0; i < leftpad; i++) {
        putchar(' ');
    }
    puts("[TCP] ");
    net_print_decimal_u16(src_port);
    puts(" -> ");
    net_print_decimal_u16(dst_port);
    puts(" seq=");
    net_print_decimal_u32(seq);
    puts(" ack=");
    net_print_decimal_u32(ack);
    puts(" flags=[");

    bool first_flag = true;
    if (tcp->flags & TCP_FLAG_SYN) {
        puts("SYN");
        first_flag = false;
    }
    if (tcp->flags & TCP_FLAG_ACK) {
        if (!first_flag) {
            puts(",");
        }
        puts("ACK");
        first_flag = false;
    }
    if (tcp->flags & TCP_FLAG_FIN) {
        if (!first_flag) {
            puts(",");
        }
        puts("FIN");
        first_flag = false;
    }
    if (tcp->flags & TCP_FLAG_RST) {
        if (!first_flag) {
            puts(",");
        }
        puts("RST");
        first_flag = false;
    }
    if (tcp->flags & TCP_FLAG_PSH) {
        if (!first_flag) {
            puts(",");
        }
        puts("PSH");
        first_flag = false;
    }
    if (tcp->flags & TCP_FLAG_URG) {
        if (!first_flag) {
            puts(",");
        }
        puts("URG");
    }

    puts("] win=");
    net_print_decimal_u16(win);
    puts(" len=");
    net_print_decimal_u16(data_offset);
    puts("\n");
}

uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t *tcp_segment, uint16_t tcp_length) {
    uint32_t sum = 0;

    // Pseudo-header: src_ip, dst_ip, zero+protocol, tcp_length (all in network byte order)
    sum += ntohs((uint16_t)(src_ip & 0xFFFF));
    sum += ntohs((uint16_t)(src_ip >> 16));
    sum += ntohs((uint16_t)(dst_ip & 0xFFFF));
    sum += ntohs((uint16_t)(dst_ip >> 16));
    sum += IPPROTO_TCP;
    sum += tcp_length;

    // Sum TCP header + payload in 16-bit words (byte-wise reads for alignment safety)
    uint16_t remaining = tcp_length;
    size_t offset = 0;
    while (remaining > 1) {
        uint16_t word = ((uint16_t)tcp_segment[offset] << 8) | tcp_segment[offset + 1];
        sum += word;
        offset += 2;
        remaining -= 2;
    }

    // Handle odd byte
    if (remaining == 1) {
        sum += ((uint16_t)tcp_segment[tcp_length - 1]) << 8;
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return htons(~sum);
}

void tcp_build_header(tcp_hdr_t *header, uint16_t src_port, uint16_t dst_port,
                      uint32_t seq, uint32_t ack, uint8_t flags, uint16_t window,
                      uint32_t src_ip, uint32_t dst_ip, uint16_t payload_length) {
    write_htons_unaligned(&header->src_port, src_port);
    write_htons_unaligned(&header->dst_port, dst_port);
    write_htonl_unaligned(&header->seq_num, seq);
    write_htonl_unaligned(&header->ack_num, ack);
    header->data_offset = (5 << 4);  // 20 bytes, no options
    header->flags = flags;
    write_htons_unaligned(&header->window, window);
    header->checksum = 0;
    header->urgent_ptr = 0;

    uint16_t tcp_length = sizeof(tcp_hdr_t) + payload_length;
    uint16_t cksum = tcp_checksum(src_ip, dst_ip, (const uint8_t *)header, tcp_length);
    header->checksum = cksum;
}
