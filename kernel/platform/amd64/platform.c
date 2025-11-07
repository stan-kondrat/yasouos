#include "common.h"
#include "../platform.h"

// Multiboot info structure (partial - only what we need)
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;      // Physical address of command line string
    // ... other fields we don't need
} __attribute__((packed)) multiboot_info_t;

// PVH start info structure
// See https://xenbits.xen.org/docs/unstable/misc/pvh.html
// See xen/include/public/arch-x86/hvm/start_info.h
typedef struct {
    uint32_t magic;         // Magic value: 0x336ec578 ("xEn3")
    uint32_t version;       // Version of this structure
    uint32_t flags;         // SIF_xxx flags
    uint32_t nr_modules;    // Number of modules passed to kernel
    uint64_t modlist_paddr; // Physical address of hvm_modlist_entry array
    uint64_t cmdline_paddr; // Physical address of command line
    uint64_t rsdp_paddr;    // Physical address of RSDP ACPI structure
    uint64_t memmap_paddr;  // Physical address of memory map (version >= 1)
    uint32_t memmap_entries;// Number of entries in memory map
    uint32_t reserved;      // Must be zero
} __attribute__((packed)) hvm_start_info_t;

// Magic values
#define MULTIBOOT_FLAG_CMDLINE (1 << 2)
#define PVH_MAGIC 0x336ec578  // PVH start info magic

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
    // QEMU returns ((exit_value << 1) | 1), so to get exit code 0:
    // We need (x << 1) | 1 = 0, which is impossible
    // So we use exit value 0x10 which gives us exit code 0x21 (33)
    outb(0xf4, 0x10);  // Exit with code 33

    // If debug exit device not available, halt
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

const char* platform_get_cmdline(uintptr_t boot_param) {
    if (boot_param == 0) {
        return NULL;
    }

    // Try PVH protocol first
    hvm_start_info_t *pvh = (hvm_start_info_t *)boot_param;
    if (pvh->magic == PVH_MAGIC) {
        if (pvh->cmdline_paddr == 0) {
            return NULL;
        }
        return (const char *)(uintptr_t)pvh->cmdline_paddr;
    }

    // Try Multiboot protocol as fallback
    multiboot_info_t *mbi = (multiboot_info_t *)boot_param;
    if ((mbi->flags & MULTIBOOT_FLAG_CMDLINE)) {
        if (mbi->cmdline == 0) {
            return NULL;
        }
        return (const char *)(uintptr_t)mbi->cmdline;
    }

    return NULL;
}