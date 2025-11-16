#pragma once

#include "../../../common/types.h"

#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_hdr_t;

/**
 * Print TCP segment header
 * @param tcp_segment Pointer to TCP segment data
 * @param length Segment length in bytes
 * @param leftpad Number of spaces to pad on the left (use 0 for no padding)
 */
void tcp_print(const uint8_t *tcp_segment, size_t length, int leftpad);
