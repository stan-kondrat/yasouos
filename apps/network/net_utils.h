#pragma once

#include "../../common/types.h"
#include "../../common/common.h"

static inline uint16_t ntohs(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint16_t htons(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint32_t ntohl(uint32_t x) {
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
}

static inline uint32_t htonl(uint32_t x) {
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
}

static inline void net_print_mac(const uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) {
        put_hex8(mac[i]);
        if (i < 5) {
            puts(":");
        }
    }
}

static inline void net_print_decimal_u8(uint8_t value) {
    if (value >= 100) {
        putchar('0' + (value / 100));
        value %= 100;
        putchar('0' + (value / 10));
        putchar('0' + (value % 10));
    } else if (value >= 10) {
        putchar('0' + (value / 10));
        putchar('0' + (value % 10));
    } else {
        putchar('0' + value);
    }
}

static inline void net_print_decimal_u16(uint16_t value) {
    char buf[6];
    int i = 0;

    if (value == 0) {
        putchar('0');
        return;
    }

    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        putchar(buf[--i]);
    }
}

static inline void net_print_decimal_u32(uint32_t value) {
    char buf[12];
    int i = 0;

    if (value == 0) {
        putchar('0');
        return;
    }

    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        putchar(buf[--i]);
    }
}

static inline void net_print_ip(uint32_t ip) {
    net_print_decimal_u8((ip >> 24) & 0xFF);
    puts(".");
    net_print_decimal_u8((ip >> 16) & 0xFF);
    puts(".");
    net_print_decimal_u8((ip >> 8) & 0xFF);
    puts(".");
    net_print_decimal_u8(ip & 0xFF);
}

