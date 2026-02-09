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

/**
 * Calculate TCP checksum over pseudo-header + TCP segment
 * @param src_ip Source IP address (network byte order)
 * @param dst_ip Destination IP address (network byte order)
 * @param tcp_segment Pointer to TCP header + payload (contiguous)
 * @param tcp_length Total length of TCP header + payload in bytes
 * @return Checksum in network byte order
 */
uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t *tcp_segment, uint16_t tcp_length);

/**
 * Build TCP header and compute checksum
 * Payload must already be in memory immediately after the header before calling.
 * @param header Pointer to TCP header structure to fill
 * @param src_port Source port (host byte order)
 * @param dst_port Destination port (host byte order)
 * @param seq Sequence number (host byte order)
 * @param ack Acknowledgment number (host byte order)
 * @param flags TCP flags (TCP_FLAG_SYN, TCP_FLAG_ACK, etc.)
 * @param window Window size (host byte order)
 * @param src_ip Source IP address (network byte order)
 * @param dst_ip Destination IP address (network byte order)
 * @param payload_length Length of payload after TCP header
 */
void tcp_build_header(tcp_hdr_t *header, uint16_t src_port, uint16_t dst_port,
                      uint32_t seq, uint32_t ack, uint8_t flags, uint16_t window,
                      uint32_t src_ip, uint32_t dst_ip, uint16_t payload_length);
