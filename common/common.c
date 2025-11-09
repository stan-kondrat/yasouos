#include "common.h"
#include "../kernel/platform/platform.h"

void* memset(void* buf, int c, size_t n) {
    uint8_t* p = (uint8_t*)buf;
    while (n--)
        *p++ = c;
    return buf;
}

void* memcpy(void* dst, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++)
        len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n > 0 && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void putchar(char ch) {
    platform_putchar(ch);
}

void puts(const char* s) {
    platform_puts(s);
}

static const char hex_chars[] = "0123456789abcdef";

void put_hex8(uint8_t value) {
    putchar(hex_chars[(value >> 4) & 0xF]);
    putchar(hex_chars[value & 0xF]);
}

void put_hex16(uint16_t value) {
    putchar(hex_chars[(value >> 12) & 0xF]);
    putchar(hex_chars[(value >> 8) & 0xF]);
    putchar(hex_chars[(value >> 4) & 0xF]);
    putchar(hex_chars[value & 0xF]);
}

void put_hex32(uint32_t value) {
    putchar(hex_chars[(value >> 28) & 0xF]);
    putchar(hex_chars[(value >> 24) & 0xF]);
    putchar(hex_chars[(value >> 20) & 0xF]);
    putchar(hex_chars[(value >> 16) & 0xF]);
    putchar(hex_chars[(value >> 12) & 0xF]);
    putchar(hex_chars[(value >> 8) & 0xF]);
    putchar(hex_chars[(value >> 4) & 0xF]);
    putchar(hex_chars[value & 0xF]);
}

void put_hex64(uint64_t value) {
    put_hex32((uint32_t)(value >> 32));
    put_hex32((uint32_t)(value & 0xFFFFFFFF));
}
