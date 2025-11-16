#pragma once

#include "../../../common/types.h"

#define IPPROTO_ICMP 1
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17

typedef struct {
    uint8_t  version_ihl;
    uint8_t  tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} __attribute__((packed)) ipv4_hdr_t;

/**
 * Print IPv4 packet header and invoke appropriate protocol handler
 * @param packet Pointer to IPv4 packet data
 * @param length Packet length in bytes
 * @param leftpad Number of spaces to pad on the left (use 0 for no padding)
 */
void ipv4_print(const uint8_t *packet, size_t length, int leftpad);

/**
 * Build IPv4 header
 * @param header Pointer to IPv4 header structure to fill
 * @param src_ip Source IP address (network byte order)
 * @param dst_ip Destination IP address (network byte order)
 * @param protocol Protocol number (IPPROTO_UDP, IPPROTO_TCP, etc.)
 * @param payload_length Length of payload after IPv4 header
 * @param ttl Time to live value
 */
void ipv4_build_header(ipv4_hdr_t *header, uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, uint16_t payload_length, uint8_t ttl);

/**
 * Calculate IPv4 header checksum
 * @param header Pointer to IPv4 header
 * @return Calculated checksum in network byte order
 */
uint16_t ipv4_checksum(const ipv4_hdr_t *header);

/**
 * Verify IPv4 header checksum
 * @param header Pointer to IPv4 header
 * @return true if checksum is valid, false otherwise
 */
bool ipv4_verify_checksum(const ipv4_hdr_t *header);
