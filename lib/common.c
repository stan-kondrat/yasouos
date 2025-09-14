#include "common.h"

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

void putchar(char ch) {
    platform_putchar(ch);
}

void puts(const char* s) {
    platform_puts(s);
}
