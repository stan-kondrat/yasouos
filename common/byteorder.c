#include "byteorder.h"

// ==============================================================================
// Unaligned-safe byte order conversion functions (32-bit)
// ==============================================================================

// ntohl_unaligned: Read 32-bit value from possibly unaligned address and convert from network to host byte order
// NOTE: This reads bytes from memory in network byte order (big-endian) and returns host byte order
uint32_t ntohl_unaligned(const void *ptr) {
    const volatile uint8_t *bytes = (const volatile uint8_t *)ptr;
    // Network byte order is always big-endian
    // Read bytes in big-endian order and let the host interpret them
    uint32_t result;
    result = ((uint32_t)bytes[0] << 24) |
             ((uint32_t)bytes[1] << 16) |
             ((uint32_t)bytes[2] << 8) |
             ((uint32_t)bytes[3]);
    return result;
}

// htonl_unaligned: Read 32-bit value from possibly unaligned address and convert from host to network byte order
// NOTE: Despite the name, this reads from memory. Use this when reading network byte order data from unaligned addresses.
// For symmetry with ntohl_unaligned, both functions perform the same operation (network->host byte order conversion).
uint32_t htonl_unaligned(const void *ptr) {
    return ntohl_unaligned(ptr);
}

// write_htonl_unaligned: Convert 32-bit value from host to network byte order and write to possibly unaligned address
void write_htonl_unaligned(void *ptr, uint32_t value) {
    volatile uint8_t *bytes = (volatile uint8_t *)ptr;
    // Always write in network byte order (big-endian)
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
}

// write_ntohl_unaligned: Convert 32-bit value from network to host byte order and write to possibly unaligned address
void write_ntohl_unaligned(void *ptr, uint32_t value) {
    write_htonl_unaligned(ptr, value);
}

// ==============================================================================
// Unaligned-safe byte order conversion functions (16-bit)
// ==============================================================================

// ntohs_unaligned: Read 16-bit value from possibly unaligned address and convert from network to host byte order
// NOTE: This reads bytes from memory in network byte order (big-endian) and returns host byte order
uint16_t ntohs_unaligned(const void *ptr) {
    const volatile uint8_t *bytes = (const volatile uint8_t *)ptr;
    // Network byte order is always big-endian
    uint16_t result;
    result = ((uint16_t)bytes[0] << 8) |
             ((uint16_t)bytes[1]);
    return result;
}

// htons_unaligned: Read 16-bit value from possibly unaligned address and convert from host to network byte order
// NOTE: Despite the name, this reads from memory. Use this when reading network byte order data from unaligned addresses.
// For symmetry with ntohs_unaligned, both functions perform the same operation (network->host byte order conversion).
uint16_t htons_unaligned(const void *ptr) {
    return ntohs_unaligned(ptr);
}

// write_htons_unaligned: Convert 16-bit value from host to network byte order and write to possibly unaligned address
void write_htons_unaligned(void *ptr, uint16_t value) {
    volatile uint8_t *bytes = (volatile uint8_t *)ptr;
    // Always write in network byte order (big-endian)
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
}

// write_ntohs_unaligned: Convert 16-bit value from host to network byte order and write to possibly unaligned address
void write_ntohs_unaligned(void *ptr, uint16_t value) {
    write_htons_unaligned(ptr, value);
}
