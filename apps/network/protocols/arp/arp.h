#pragma once

#include "../../../../common/types.h"

#define ARP_HW_ETHER    1
#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

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
 */
void arp_print(const arp_hdr_t *arp);
