.section .text.boot
.global _start

_start:
    // a0 and a1 contain boot parameters from OpenSBI
    // a0 = hart ID, a1 = device tree pointer
    // Save both before they get clobbered
    mv s0, a0      // Save hart ID (for debugging)
    mv s1, a1      // Save device tree pointer

    // Setup stack pointer
    la sp, __stack_top

    // Clear frame pointer
    li fp, 0

    // Clear BSS section
    la t0, __bss
    la t1, __bss_end
1:  bge t0, t1, 2f
    sd zero, (t0)
    addi t0, t0, 8
    j 1b
2:

    // Pass device tree pointer as first argument to kernel_main
    mv a0, s1      // Pass FDT pointer (was in a1 at entry)
    call kernel_main

    // Should never return, but if it does, halt
1:  wfi
    j 1b