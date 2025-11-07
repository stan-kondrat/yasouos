# Simple multiboot header for direct boot
.section .multiboot
.align 4
.long 0x1BADB002    # magic
.long 0x00000003    # flags
.long -(0x1BADB002 + 0x00000003)  # checksum

# PVH ELF note for modern QEMU direct kernel loading
.section .note.xen, "a"
.align 4
.long 4              # namesz (length of "Xen")
.long 12             # descsz (3 uint32_t values)
.long 18             # type (XEN_ELFNOTE_PHYS32_ENTRY = 18)
.ascii "Xen\0"       # name
.long _start         # 32-bit entry point
.long 0              # reserved
.long 0              # reserved

.section .text.boot
.global _start
.code32

_start:
    # We start in 32-bit mode from multiboot
    # ebx contains multiboot info pointer - save it
    cli

    # Save multiboot info pointer before clobbering ebx
    movl %ebx, %esi

    # Setup stack pointer
    movl $__stack_top, %esp

    # Clear BSS section
    movl $__bss, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    # Pass multiboot info pointer as first argument to kernel_main
    pushl %esi
    call kernel_main
    addl $4, %esp

    # Should never return, but if it does, halt
1:  hlt
    jmp 1b

