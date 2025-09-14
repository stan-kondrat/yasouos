.section .text.boot
.global _start

_start:
    // Setup stack pointer
    la sp, __stack_top

    // Clear frame pointer
    li fp, 0

    // Jump to kernel main
    call kernel_main

    // Should never return, but if it does, halt
1:  wfi
    j 1b