#pragma once

#include "types.h"

// ==============================================================================
// RISC-V Implementation
// ==============================================================================
#if defined(__riscv)

// htonl_riscv: Convert 32-bit value from host to network byte order (RISC-V)
static inline uint32_t htonl_riscv(uint32_t x) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;
#else
    uint32_t result;
    __asm__ (
        "srliw t0, %1, 24\n\t"          // Byte 3 -> position 0 (32-bit shift)
        "slliw t1, %1, 24\n\t"          // Byte 0 -> position 3 (32-bit shift)
        "or    t0, t0, t1\n\t"          // Combine outer bytes
        "srliw t1, %1, 16\n\t"          // Shift to get byte 2 (32-bit shift)
        "andi  t1, t1, 0xff\n\t"        // Mask byte 2
        "slli  t1, t1, 8\n\t"           // Byte 2 -> position 1
        "or    t0, t0, t1\n\t"          // Combine
        "srliw t1, %1, 8\n\t"           // Shift to get byte 1 (32-bit shift)
        "andi  t1, t1, 0xff\n\t"        // Mask byte 1
        "slli  t1, t1, 16\n\t"          // Byte 1 -> position 2
        "or    %0, t0, t1\n\t"          // Final result
        "slli  %0, %0, 32\n\t"          // Clear upper 32 bits
        "srli  %0, %0, 32"              // Zero-extend to 64-bit
        : "=r" (result)
        : "r" (x)
        : "t0", "t1"
    );
    return result;
#endif
}

// htons_riscv: Convert 16-bit value from host to network byte order (RISC-V)
static inline uint16_t htons_riscv(uint16_t x) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;
#else
    uint16_t result;
    __asm__ (
        "andi t0, %1, 0xff\n\t"         // Get low byte
        "slli t0, t0, 8\n\t"            // Shift to high position
        "srli t1, %1, 8\n\t"            // Get high byte
        "andi t1, t1, 0xff\n\t"         // Mask to byte
        "or   %0, t0, t1"               // Combine
        : "=r" (result)
        : "r" (x)
        : "t0", "t1"
    );
    return result;
#endif
}

// ntohl_riscv: Convert 32-bit value from network to host byte order (RISC-V)
static inline uint32_t ntohl_riscv(uint32_t x) {
    return htonl_riscv(x);
}

// ntohs_riscv: Convert 16-bit value from network to host byte order (RISC-V)
static inline uint16_t ntohs_riscv(uint16_t x) {
    return htons_riscv(x);
}

// ==============================================================================
// ARM64 (AArch64) Implementation
// ==============================================================================
#elif defined(__aarch64__) || defined(__arm64__)

// htonl_arm64: Convert 32-bit value from host to network byte order (ARM64)
static inline uint32_t htonl_arm64(uint32_t x) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;
#else
    uint32_t result;
    __asm__ (
        "rev %w0, %w1"                  // REV reverses byte order in 32-bit register
        : "=r" (result)
        : "r" (x)
    );
    return result;
#endif
}

// htons_arm64: Convert 16-bit value from host to network byte order (ARM64)
static inline uint16_t htons_arm64(uint16_t x) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;
#else
    uint16_t result;
    __asm__ (
        "rev16 %w0, %w1"                // REV16 reverses bytes in each 16-bit halfword
        : "=r" (result)
        : "r" (x)
    );
    return result;
#endif
}

// ntohl_arm64: Convert 32-bit value from network to host byte order (ARM64)
static inline uint32_t ntohl_arm64(uint32_t x) {
    return htonl_arm64(x);
}

// ntohs_arm64: Convert 16-bit value from network to host byte order (ARM64)
static inline uint16_t ntohs_arm64(uint16_t x) {
    return htons_arm64(x);
}

// ==============================================================================
// AMD64 (x86-64) Implementation
// ==============================================================================
#elif defined(__x86_64__) || defined(__amd64__)

// htonl_amd64: Convert 32-bit value from host to network byte order (AMD64)
static inline uint32_t htonl_amd64(uint32_t x) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;
#else
    uint32_t result;
    __asm__ (
        "bswap %0"                      // BSWAP reverses byte order in 32-bit register
        : "=r" (result)
        : "0" (x)
    );
    return result;
#endif
}

// htons_amd64: Convert 16-bit value from host to network byte order (AMD64)
static inline uint16_t htons_amd64(uint16_t x) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;
#else
    uint16_t result;
    __asm__ (
        "xchg %b0, %h0"                 // Exchange low and high bytes
        : "=Q" (result)
        : "0" (x)
    );
    return result;
#endif
}

// ntohl_amd64: Convert 32-bit value from network to host byte order (AMD64)
static inline uint32_t ntohl_amd64(uint32_t x) {
    return htonl_amd64(x);
}

// ntohs_amd64: Convert 16-bit value from network to host byte order (AMD64)
static inline uint16_t ntohs_amd64(uint16_t x) {
    return htons_amd64(x);
}

#else
#error "Unsupported architecture"
#endif

// ==============================================================================
// Public API - inline wrappers to architecture-specific implementations
// ==============================================================================

// htonl: Convert 32-bit value from host to network byte order (public API)
static inline uint32_t htonl(uint32_t x) {
#if defined(__riscv)
    return htonl_riscv(x);
#elif defined(__aarch64__) || defined(__arm64__)
    return htonl_arm64(x);
#elif defined(__x86_64__) || defined(__amd64__)
    return htonl_amd64(x);
#else
    #error "Unsupported architecture for byte order conversion"
#endif
}

// htons: Convert 16-bit value from host to network byte order (public API)
static inline uint16_t htons(uint16_t x) {
#if defined(__riscv)
    return htons_riscv(x);
#elif defined(__aarch64__) || defined(__arm64__)
    return htons_arm64(x);
#elif defined(__x86_64__) || defined(__amd64__)
    return htons_amd64(x);
#else
    #error "Unsupported architecture for byte order conversion"
#endif
}

// ntohl: Convert 32-bit value from network to host byte order (public API)
static inline uint32_t ntohl(uint32_t x) {
#if defined(__riscv)
    return ntohl_riscv(x);
#elif defined(__aarch64__) || defined(__arm64__)
    return ntohl_arm64(x);
#elif defined(__x86_64__) || defined(__amd64__)
    return ntohl_amd64(x);
#else
    #error "Unsupported architecture for byte order conversion"
#endif
}

// ntohs: Convert 16-bit value from network to host byte order (public API)
static inline uint16_t ntohs(uint16_t x) {
#if defined(__riscv)
    return ntohs_riscv(x);
#elif defined(__aarch64__) || defined(__arm64__)
    return ntohs_arm64(x);
#elif defined(__x86_64__) || defined(__amd64__)
    return ntohs_amd64(x);
#else
    #error "Unsupported architecture for byte order conversion"
#endif
}

// ==============================================================================
// Unaligned-safe byte order conversion functions (32-bit)
// ==============================================================================

uint32_t ntohl_unaligned(const void *ptr);
uint32_t htonl_unaligned(const void *ptr);
void write_htonl_unaligned(void *ptr, uint32_t value);
void write_ntohl_unaligned(void *ptr, uint32_t value);

// ==============================================================================
// Unaligned-safe byte order conversion functions (16-bit)
// ==============================================================================

uint16_t ntohs_unaligned(const void *ptr);
uint16_t htons_unaligned(const void *ptr);
void write_htons_unaligned(void *ptr, uint16_t value);
void write_ntohs_unaligned(void *ptr, uint16_t value);

