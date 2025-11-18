// app_illegal_instruction.c - Test application that executes an illegal instruction
#include "types.h"

void app_illegal_instruction(void) {
#if defined(__aarch64__)
    // ARM64: Use an undefined instruction (UDF #0)
    __asm__ volatile(".word 0x00000000");
#elif defined(__x86_64__) || defined(__i386__) || defined(__i686__)
    // AMD64/x86: Use UD2 instruction (undefined instruction)
    __asm__ volatile("ud2");
#elif defined(__riscv)
    // RISC-V: Use an illegal instruction (all zeros)
    __asm__ volatile(".word 0x00000000");
#else
    #error "Unsupported architecture"
#endif

    // Should never reach here
    __builtin_unreachable();
}
