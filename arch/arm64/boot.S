.section .text.boot
.global _start

_start:
    // Setup stack pointer
    adrp x0, __stack_top
    add sp, x0, :lo12:__stack_top

    // Jump to kernel main
    bl kernel_main

    // Should never return, but if it does, halt
1:  wfe
    b 1b