#pragma once

#include "../../../common/types.h"

#define ARP_HW_ETHER    1
#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

#define ARP_PACKET_SIZE 42
#define BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_len;
    uint8_t  proto_len;
    uint16_t opcode;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) arp_hdr_t;

/**
 * Print ARP packet details
 * @param arp Pointer to ARP header
 * @param leftpad Number of spaces to pad on the left (use 0 for no padding)
 */
void arp_print(const arp_hdr_t *arp, int leftpad);

/**
 * Build ARP request packet with Ethernet header
 * @param packet Buffer to write packet (must be at least ARP_PACKET_SIZE bytes)
 * @param sender_mac Sender MAC address
 * @param sender_ip Sender IP address (host byte order)
 * @param target_ip Target IP address (host byte order)
 */
void arp_build_request(uint8_t *packet,
                       const uint8_t sender_mac[6],
                       uint32_t sender_ip,
                       uint32_t target_ip);

/**
 * Build ARP reply packet with Ethernet header
 * @param packet Buffer to write packet (must be at least ARP_PACKET_SIZE bytes)
 * @param sender_mac Sender MAC address (our MAC)
 * @param sender_ip Sender IP address (our IP, host byte order)
 * @param target_mac Target MAC address (requester's MAC)
 * @param target_ip Target IP address (requester's IP, host byte order)
 */
void arp_build_reply(uint8_t *packet,
                     const uint8_t sender_mac[6],
                     uint32_t sender_ip,
                     const uint8_t target_mac[6],
                     uint32_t target_ip);

/**
 * Parse ARP packet with Ethernet header
 * @param packet Pointer to packet data
 * @param length Packet length in bytes
 * @param out_arp Pointer to store parsed ARP header (optional, can be NULL)
 * @return Pointer to ARP header in packet, or NULL if invalid
 */
const arp_hdr_t* arp_parse(const uint8_t *packet, size_t length, arp_hdr_t *out_arp);

