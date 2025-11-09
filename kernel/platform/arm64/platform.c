#include "common.h"
#include "../platform.h"
#include "../fdt_parser.h"

// Forward declarations
static void platform_setup_exception_handlers(void);

void platform_init(void)
{
    // QEMU virt machine PL011 UART is pre-initialized

    // Setup exception handlers
    platform_setup_exception_handlers();
}

void platform_putchar(char ch)
{
    // Direct PL011 UART access - QEMU virt machine UART at 0x09000000
    volatile uint32_t *uart = (volatile uint32_t *)0x09000000;
    *uart = ch;
}

void platform_puts(const char *str)
{
    while (*str)
    {
        platform_putchar(*str++);
    }
}

void platform_halt(void)
{
    // Use PSCI (Power State Coordination Interface) system shutdown
    register long x0 __asm__("x0") = 0x84000008; // PSCI_SYSTEM_OFF
    __asm__ volatile("hvc #0" : : "r"(x0) : "memory");

    // Fallback: infinite loop
    while (1)
    {
        __asm__ volatile("wfe");
    }
}

const char* platform_get_cmdline(uintptr_t boot_param) {
    // Use FDT parser to extract bootargs from device tree
    return fdt_get_bootargs(boot_param);
}

// Exception handler for unknown instruction
void exception_unknown_instruction(void) {
    uint64_t elr;
    uint64_t esr;
    uint32_t instruction;

    // Read ELR_EL1 (Exception Link Register - contains the faulting PC)
    __asm__ volatile("mrs %0, elr_el1" : "=r"(elr));

    // Read ESR_EL1 (Exception Syndrome Register - contains exception info)
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));

    // Read the instruction at the faulting address
    instruction = *(volatile uint32_t*)elr;

    uint32_t ec = (esr >> 26) & 0x3F; // Exception Class
    uint32_t iss = esr & 0x1FFFFFF;   // Instruction Specific Syndrome

    puts("\n[EXCEPTION] ");
    if (ec == 0x00) {
        puts("Unknown/Illegal Instruction\n");
    } else if (ec == 0x0E) {
        puts("Illegal Instruction\n");
    } else if (ec == 0x24 || ec == 0x25) {
        puts("Data Abort\n");
    } else if (ec == 0x20 || ec == 0x21) {
        puts("Instruction Abort\n");
    } else {
        puts("Unknown Exception\n");
    }
    puts("EC=0x");
    put_hex8((uint8_t)ec);
    puts(" ISS=0x");
    put_hex32(iss);
    puts("\nAddress: 0x");
    put_hex32((uint32_t)(elr >> 32));
    put_hex32((uint32_t)elr);
    puts("\nInstruction: 0x");
    put_hex32(instruction);
    puts("\nSystem halted.\n");
    platform_halt();
}

// Assembly stubs for exception vectors
__asm__(
    ".align 11\n"
    ".global exception_vector_base\n"
    "exception_vector_base:\n"

    // Current EL with SP0
    ".align 7\n"
    "b exception_sync_el1t\n"
    ".align 7\n"
    "b exception_irq_el1t\n"
    ".align 7\n"
    "b exception_fiq_el1t\n"
    ".align 7\n"
    "b exception_serror_el1t\n"

    // Current EL with SPx
    ".align 7\n"
    "b exception_sync_el1h\n"
    ".align 7\n"
    "b exception_irq_el1h\n"
    ".align 7\n"
    "b exception_fiq_el1h\n"
    ".align 7\n"
    "b exception_serror_el1h\n"

    // Lower EL using AArch64
    ".align 7\n"
    "b exception_sync_el0_64\n"
    ".align 7\n"
    "b exception_irq_el0_64\n"
    ".align 7\n"
    "b exception_fiq_el0_64\n"
    ".align 7\n"
    "b exception_serror_el0_64\n"

    // Lower EL using AArch32
    ".align 7\n"
    "b exception_sync_el0_32\n"
    ".align 7\n"
    "b exception_irq_el0_32\n"
    ".align 7\n"
    "b exception_fiq_el0_32\n"
    ".align 7\n"
    "b exception_serror_el0_32\n"

    // Exception handlers - all call the C handler
    "exception_sync_el1t:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_irq_el1t:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_fiq_el1t:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_serror_el1t:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_sync_el1h:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_irq_el1h:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_fiq_el1h:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_serror_el1h:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_sync_el0_64:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_irq_el0_64:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_fiq_el0_64:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_serror_el0_64:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_sync_el0_32:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_irq_el0_32:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_fiq_el0_32:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"

    "exception_serror_el0_32:\n"
    "    bl exception_unknown_instruction\n"
    "    b .\n"
);

extern void exception_vector_base(void);

static void platform_setup_exception_handlers(void) {
    // Set VBAR_EL1 to point to our exception vector table
    __asm__ volatile("msr vbar_el1, %0" : : "r"(exception_vector_base));

    puts("[ARM64] Exception handlers installed\n");
}
