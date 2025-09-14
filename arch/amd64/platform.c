#include "common.h"

// I/O port operations
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Serial port (COM1) registers
#define COM1_BASE 0x3F8
#define COM1_DATA COM1_BASE
#define COM1_LSR  (COM1_BASE + 5)
#define LSR_THRE  0x20  // Transmitter holding register empty

void platform_init(void) {
    // Initialize COM1 serial port
    outb(COM1_BASE + 1, 0x00);  // Disable interrupts
    outb(COM1_BASE + 3, 0x80);  // Enable DLAB (set baud rate divisor)
    outb(COM1_BASE + 0, 0x03);  // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_BASE + 1, 0x00);  // (hi byte)
    outb(COM1_BASE + 3, 0x03);  // 8 bits, no parity, one stop bit
    outb(COM1_BASE + 2, 0xC7);  // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_BASE + 4, 0x0B);  // IRQs enabled, RTS/DSR set
}

void platform_putchar(char ch) {
    // Wait for transmitter to be ready
    while ((inb(COM1_LSR) & LSR_THRE) == 0);

    // Send character
    outb(COM1_DATA, ch);
}

void platform_puts(const char *str) {
    while (*str) {
        platform_putchar(*str++);
    }
}

void platform_halt(void) {
    // Use QEMU ISA debug exit device (iobase=0xf4)
    // Exit code 0 = success, any other value = ((code << 1) | 1)
    outb(0xf4, 0x00);  // Exit with code 0

    // If debug exit device not available, halt
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}