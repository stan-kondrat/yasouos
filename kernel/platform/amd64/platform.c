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

// Forward declaration
static void platform_setup_exception_handlers(void);

void platform_init(void) {
    // Initialize COM1 serial port
    outb(COM1_BASE + 1, 0x00);  // Disable interrupts
    outb(COM1_BASE + 3, 0x80);  // Enable DLAB (set baud rate divisor)
    outb(COM1_BASE + 0, 0x03);  // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_BASE + 1, 0x00);  // (hi byte)
    outb(COM1_BASE + 3, 0x03);  // 8 bits, no parity, one stop bit
    outb(COM1_BASE + 2, 0xC7);  // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_BASE + 4, 0x0B);  // IRQs enabled, RTS/DSR set

    // Setup exception handlers
    platform_setup_exception_handlers();
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

// IDT entry structure (64-bit)
typedef struct {
    uint16_t offset_low;    // Offset bits 0-15
    uint16_t selector;      // Code segment selector
    uint8_t  ist;           // Interrupt Stack Table offset
    uint8_t  type_attr;     // Type and attributes
    uint16_t offset_mid;    // Offset bits 16-31
    uint32_t offset_high;   // Offset bits 32-63
    uint32_t reserved;      // Reserved (must be 0)
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

// IDT with 256 entries
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

// Exception handler for invalid opcode (#UD - exception 6)
void exception_invalid_opcode_handler(void) {
    puts("\n[EXCEPTION] Unknown/Illegal Instruction\n");
    puts("The CPU encountered an instruction it does not recognize.\n");
    puts("System halted.\n");
    platform_halt();
}

// Assembly stub for invalid opcode exception
// The CPU automatically pushes error code and return address
__asm__(
    ".global exception_stub_ud\n"
    ".align 16\n"
    "exception_stub_ud:\n"
    // No error code for #UD, so we don't need to pop it
    // Save registers
    "    pushq %rax\n"
    "    pushq %rbx\n"
    "    pushq %rcx\n"
    "    pushq %rdx\n"
    "    pushq %rsi\n"
    "    pushq %rdi\n"
    "    pushq %rbp\n"
    "    pushq %r8\n"
    "    pushq %r9\n"
    "    pushq %r10\n"
    "    pushq %r11\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    // Call C handler
    "    call exception_invalid_opcode_handler\n"
    // Should never return, but if it does, restore and iret
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %r11\n"
    "    popq %r10\n"
    "    popq %r9\n"
    "    popq %r8\n"
    "    popq %rbp\n"
    "    popq %rdi\n"
    "    popq %rsi\n"
    "    popq %rdx\n"
    "    popq %rcx\n"
    "    popq %rbx\n"
    "    popq %rax\n"
    "    iretq\n"
);

extern void exception_stub_ud(void);

// Set IDT entry
static void idt_set_entry(uint8_t vector, uint64_t handler, uint16_t selector, uint8_t type_attr) {
    idt[vector].offset_low = handler & 0xFFFF;
    idt[vector].offset_mid = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].type_attr = type_attr;
    idt[vector].reserved = 0;
}

static void platform_setup_exception_handlers(void) {
    // Clear IDT - avoid memset due to potential issues with large buffers
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].reserved = 0;
    }

    // Set up IDT entry for invalid opcode (#UD - exception 6)
    // Type: 0x8E = Present, DPL=0, Type=Interrupt Gate (64-bit)
    // Selector: 0x18 = 64-bit Code segment (assuming GDT layout: null, code32, data32, code64)
    // Note: This assumes the GDT has 64-bit code segment at index 3 (0x18 = 3 * 8)
    idt_set_entry(6, (uint64_t)(uintptr_t)exception_stub_ud, 0x18, 0x8E);

    // Load IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)(uintptr_t)&idt;
    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));

    puts("[AMD64] Exception handlers installed\n");
}
