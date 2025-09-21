# Simple multiboot header for direct boot
.section .multiboot
.align 4
.long 0x1BADB002    # magic
.long 0x00000003    # flags
.long -(0x1BADB002 + 0x00000003)  # checksum

.section .text.boot
.global _start
.code32

_start:
    # We start in 32-bit mode from multiboot
    cli

    # Setup stack pointer
    movl $__stack_top, %esp

    # Clear BSS section
    movl $__bss, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    # Call kernel main (staying in 32-bit mode for QEMU compatibility)
    call kernel_main

    # Should never return, but if it does, halt
1:  hlt
    jmp 1b

