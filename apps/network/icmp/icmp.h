#pragma once

#include "../../../common/types.h"

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed)) icmp_hdr_t;

/**
 * Print ICMP packet header
 * @param packet Pointer to ICMP packet data
 * @param length Packet length in bytes
 * @param leftpad Number of spaces to pad on the left (use 0 for no padding)
 */
void icmp_print(const uint8_t *packet, size_t length, int leftpad);

/**
 * Build ICMP echo request packet
 * @param header Pointer to ICMP header structure to fill
 * @param id Identifier field
 * @param sequence Sequence number field
 */
void icmp_build_request(icmp_hdr_t *header, uint16_t id, uint16_t sequence);

/**
 * Build ICMP echo response packet
 * @param header Pointer to ICMP header structure to fill
 * @param id Identifier field
 * @param sequence Sequence number field
 */
void icmp_build_response(icmp_hdr_t *header, uint16_t id, uint16_t sequence);

/**
 * Parse ICMP packet header
 * @param packet Pointer to ICMP packet data
 * @param length Packet length in bytes
 * @param type Output: ICMP type
 * @param code Output: ICMP code
 * @param id Output: Identifier
 * @param sequence Output: Sequence number
 * @return true if packet is valid, false otherwise
 */
bool icmp_parse(const uint8_t *packet, size_t length, uint8_t *type, uint8_t *code, uint16_t *id, uint16_t *sequence);
