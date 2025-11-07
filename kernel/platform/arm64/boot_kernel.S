.section .text.boot
.global _start

// ARM64 Linux Image header (required for QEMU to pass DTB in x0)
// See Documentation/arm64/booting.rst in Linux kernel
_start:
    b       _real_start              // branch to kernel start, magic
    .long   0                         // reserved
    .quad   0x200000                  // Image load offset from start of RAM (2MB)
    .quad   _end - _start             // Effective Image size
    .quad   0                         // kernel flags
    .quad   0                         // reserved
    .quad   0                         // reserved
    .quad   0                         // reserved
    .byte   0x41                      // Magic number, "ARM\x64"
    .byte   0x52
    .byte   0x4d
    .byte   0x64
    .long   0                         // reserved (PE COFF offset)

_real_start:
    // x0 contains device tree pointer from QEMU - save it in callee-saved register
    mov x19, x0

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

    // Pass device tree pointer as first argument to kernel_main
    mov x0, x19
    bl kernel_main

    // Should never return, but if it does, halt
1:  wfe
    b 1b