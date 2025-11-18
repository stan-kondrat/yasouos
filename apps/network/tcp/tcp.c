#include "tcp.h"
#include "../net_utils.h"

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
