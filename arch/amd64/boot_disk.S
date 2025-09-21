# Complete disk boot solution - bootloader at 0x7C00 + kernel at 0x8000

.ifdef BOOTLOADER_ONLY
    .section .text
.else
    .section .bootloader
.endif
.code16
.global _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp

    # Load kernel from disk (sector 2 onwards)
    movb $0x02, %ah
    movb $64, %al
    movb $0, %ch
    movb $2, %cl
    movb $0, %dh
    movb $0x80, %dl
    movw $0x8000, %bx
    int $0x13

    jc disk_error

    ljmp $0x0000, $0x8000

disk_error:
    movw $error_msg, %si
    call print_string_boot
    jmp .

print_string_boot:
    lodsb
    orb %al, %al
    jz .done_boot
    movb $0x0E, %ah
    int $0x10
    jmp print_string_boot
.done_boot:
    ret

error_msg:
    .asciz "Disk error!"

.org 510
.word 0xAA55

.ifndef BOOTLOADER_ONLY
# Kernel entry point at 0x8000 (separate section)
.section .text.boot16
.code16
.global _start16

_start16:
    # We're loaded at 0x8000 in real mode
    cli

    # Set up segments
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x9000, %sp

    # Print boot message in real mode
    movw $boot_msg, %si
    call print_string16

    # Enable A20 line
    inb $0x92, %al
    orb $2, %al
    outb %al, $0x92

    # Load GDT
    lgdt gdtptr

    # Switch to protected mode
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0

    # Far jump to 32-bit code
    ljmp $0x08, $protected_mode

print_string16:
    lodsb
    orb %al, %al
    jz .done
    movb $0x0E, %ah
    int $0x10
    jmp print_string16
.done:
    ret

boot_msg:
    .asciz "YasouOS booting...\r\n"

.align 8
gdt:
    # NULL descriptor
    .quad 0

    # Code segment (0x08)
    .word 0xFFFF      # limit low
    .word 0x0000      # base low
    .byte 0x00        # base middle
    .byte 0x9A        # access
    .byte 0xCF        # flags + limit high
    .byte 0x00        # base high

    # Data segment (0x10)
    .word 0xFFFF      # limit low
    .word 0x0000      # base low
    .byte 0x00        # base middle
    .byte 0x92        # access
    .byte 0xCF        # flags + limit high
    .byte 0x00        # base high

gdtptr:
    .word gdt_end - gdt - 1
    .long gdt
gdt_end:

.code32
protected_mode:
    # Set up protected mode segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Set up stack
    movl $__stack_top, %esp

    # Clear BSS
    movl $__bss, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    # Call kernel
    call kernel_main

    # Halt if kernel returns
1:  hlt
    jmp 1b
.endif