.section .text.boot
.global _start

_start:
    // Setup stack pointer
    adrp x0, __stack_top
    add sp, x0, :lo12:__stack_top

    // Clear BSS section
    adrp x0, __bss
    add x0, x0, :lo12:__bss
    adrp x1, __bss_end
    add x1, x1, :lo12:__bss_end
1:  cmp x0, x1
    bge 2f
    str xzr, [x0], #8
    b 1b
2:

    // Jump to kernel main
    bl kernel_main

    // Should never return, but if it does, halt
1:  wfe
    b 1b