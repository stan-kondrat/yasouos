.section .text.boot
.global _start

_start:
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

    // Jump to kernel main
    call kernel_main

    // Should never return, but if it does, halt
1:  wfi
    j 1b