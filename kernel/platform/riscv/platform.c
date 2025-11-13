#include "common.h"
#include "../platform.h"
#include "../fdt_parser.h"

// Forward declaration
static void platform_setup_exception_handlers(void);

void platform_init(void) {
    // Platform initialization complete - using direct hardware access

    // Setup exception handlers
    platform_setup_exception_handlers();
}

void platform_putchar(char ch) {
    // Direct UART access - QEMU virt machine UART at 0x10000000
    volatile uint8_t *uart = (volatile uint8_t *)0x10000000;
    *uart = ch;
}

void platform_puts(const char* str) {
    while (*str) {
        platform_putchar(*str++);
    }
}

void platform_halt(void) {
    // QEMU test device shutdown
    volatile uint32_t *test_device = (volatile uint32_t *)0x100000;
    *test_device = 0x5555;
}

const char* platform_get_cmdline(uintptr_t boot_param) {
    // Use FDT parser to extract bootargs from device tree
    return fdt_get_bootargs(boot_param);
}

// Exception handler for illegal instruction
void trap_illegal_instruction_handler(void) {
    puts("\n[EXCEPTION] Unknown/Illegal Instruction\n");
    puts("The CPU encountered an instruction it does not recognize.\n");
    puts("System halted.\n");
    platform_halt();
}

// General trap handler
void trap_handler(void) {
    unsigned long scause;

    // Read scause register to determine trap cause
    __asm__ volatile(
        ".option push\n"
        ".option arch, +zicsr\n"
        "csrr %0, scause\n"
        ".option pop\n"
        : "=r"(scause)
    );

    // Extract exception code (bits 0-62, bit 63 is interrupt flag)
    unsigned long exception_code = scause & 0x7FFFFFFFFFFFFFFF;

    // Exception code 2 = Illegal instruction
    if (exception_code == 2) {
        trap_illegal_instruction_handler();
    } else {
        // Handle other exceptions
        puts("\n[TRAP] Unexpected trap\n");
        puts("SCAUSE: 0x");
        put_hex32((uint32_t)(scause >> 32));
        put_hex32((uint32_t)scause);
        puts("\nSystem halted.\n");
        platform_halt();
    }
}

// Forward declaration for trap vector
extern void trap_vector(void);

// Assembly trap vector
__asm__(
    ".align 4\n"
    ".global trap_vector\n"
    "trap_vector:\n"
    "    addi sp, sp, -16\n"
    "    sd ra, 0(sp)\n"
    "    sd a0, 8(sp)\n"
    "    call trap_handler\n"
    "    ld a0, 8(sp)\n"
    "    ld ra, 0(sp)\n"
    "    addi sp, sp, 16\n"
    "1:  j 1b\n"  // Infinite loop instead of sret - trap_handler should never return
);

static void platform_setup_exception_handlers(void) {
    // Set stvec (Supervisor Trap Vector) to our trap handler
    // RISC-V boots in S-mode under OpenSBI, so we use S-mode CSRs
    unsigned long stvec_value = ((unsigned long)trap_vector);

    // Set stvec in Direct mode (bit 0-1 = 0)
    __asm__ volatile(
        ".option push\n"
        ".option arch, +zicsr\n"
        "csrw stvec, %0\n"
        ".option pop\n"
        : : "r"(stvec_value)
    );

    puts("[RISC-V] Exception handlers installed\n");
}
