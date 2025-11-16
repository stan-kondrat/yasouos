#pragma once

#include "../../../common/types.h"

#define ETH_P_ARP   0x0806
#define ETH_P_IP    0x0800

typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

/**
 * Print Ethernet frame header
 * @param frame Pointer to Ethernet frame data
 * @param length Frame length in bytes
 * @param resource Resource to print tag for (optional, can be NULL)
 * @param leftpad Number of spaces to pad on the left (use 0 for no padding)
 */
void ethernet_print(const uint8_t *frame, size_t length, const void *resource, int leftpad);
