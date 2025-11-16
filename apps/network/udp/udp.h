#pragma once

#include "../../../common/types.h"

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_hdr_t;

/**
 * Print UDP datagram header and hexdump of entire IP packet
 * @param ip_packet Pointer to IPv4 packet containing UDP datagram
 * @param length Total packet length in bytes (including IPv4 header)
 * @param leftpad Number of spaces to pad on the left (use 0 for no padding)
 */
void udp_print(const uint8_t *ip_packet, size_t length, int leftpad);

/**
 * Build UDP header
 * @param header Pointer to UDP header structure to fill
 * @param src_port Source port (host byte order)
 * @param dst_port Destination port (host byte order)
 * @param payload_length Length of payload after UDP header
 */
void udp_build_header(udp_hdr_t *header, uint16_t src_port, uint16_t dst_port, uint16_t payload_length);
