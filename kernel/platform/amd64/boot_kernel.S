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
.long 4              # descsz (size of entry point, 32-bit)
.long 18             # type (XEN_ELFNOTE_PHYS32_ENTRY = 18)
.ascii "Xen\0"       # name
.long _start         # 32-bit entry point address

.section .text.boot
.global _start
.code32

_start:
    # We start in 32-bit mode from multiboot/PVH
    # ebx contains multiboot info pointer or PVH start_info pointer - save it
    cli

    # Copy multiboot/PVH info to safe location NOW (while in 32-bit mode)
    # The info structure is in low memory and we need to preserve it
    testl %ebx, %ebx
    jz .no_boot_info

    # Copy 256 bytes from boot info structure to our safe buffer
    movl %ebx, %esi              # Source
    movl $boot_info_copy, %edi   # Destination
    movl $64, %ecx               # Copy 64 dwords (256 bytes)
    rep movsl

    # Mark that we have boot info (non-zero)
    movl $1, %edi
    jmp .have_boot_info

.no_boot_info:
    xorl %edi, %edi              # No boot info (zero)

.have_boot_info:
    # Set up identity paging for first 4GB (for PCI MMIO)
    # PML4 at 0x1000, need 24KB total (PML4 + PDPT + 4*PD)
    pushl %edi                   # Push 1 if we have boot info, 0 if not
    movl $0x1000, %edi
    movl %edi, %cr3
    xorl %eax, %eax
    movl $6144, %ecx             # Clear 24KB (6144 dwords)
    rep stosl
    movl %cr3, %edi

    # PML4[0] -> PDPT
    movl $0x2003, (%edi)
    addl $0x1000, %edi

    # PDPT[0] -> PD0 (first 1GB)
    movl $0x3003, (%edi)
    # PDPT[1] -> PD1 (second 1GB)
    movl $0x4003, 8(%edi)
    # PDPT[2] -> PD2 (third 1GB)
    movl $0x5003, 16(%edi)
    # PDPT[3] -> PD3 (fourth 1GB)
    movl $0x6003, 24(%edi)

    # Map first 4GB using 2MB pages
    # PD0 at 0x3000
    movl $0x3000, %edi
    movl $0x0083, %eax          # 0x0083 = Present, R/W, 2MB page
    movl $2048, %ecx            # 2048 * 2MB = 4GB
.fill_pd:
    movl %eax, (%edi)
    addl $0x200000, %eax        # Next 2MB
    addl $8, %edi
    loop .fill_pd

    popl %edi

    # Enable PAE
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4

    # Enable long mode in EFER MSR
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    # Enable paging
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    # Load 64-bit GDT
    lgdt gdtptr64

    # Far jump to 64-bit code using retf (pops CS:EIP)
    pushl $0x18
    pushl $long_mode
    lret

.align 8
gdt:
    # NULL descriptor
    .quad 0

    # 32-bit Code segment (0x08) - unused but for compatibility
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x9A
    .byte 0xCF
    .byte 0x00

    # 32-bit Data segment (0x10) - unused but for compatibility
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x92
    .byte 0xCF
    .byte 0x00

    # 64-bit Code segment (0x18)
    .word 0x0000
    .word 0x0000
    .byte 0x00
    .byte 0x9A
    .byte 0x20
    .byte 0x00

    # 64-bit Data segment (0x20)
    .word 0x0000
    .word 0x0000
    .byte 0x00
    .byte 0x92
    .byte 0x00
    .byte 0x00

gdtptr64:
    .word gdt_end - gdt - 1
    .quad gdt
gdt_end:

.section .data
.align 8
boot_info_copy:
    .skip 256    # Buffer to copy boot info if it's in BSS

.code64
long_mode:
    # Set up 64-bit segments
    movw $0x20, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Setup stack pointer
    movq $__stack_top, %rsp

    # Restore boot info flag from safe location (pushed at line 47)
    # Use stack manipulation to recover %edi saved before entering 32-bit paging setup
    # Since we're now in 64-bit mode after mode switch, recover from saved location
    # Actually, %edi was NOT pushed on the 64-bit stack - it was lost during mode switch
    # Instead, use boot_info_copy address if it contains valid data
    # Check first dword of boot_info_copy - if non-zero, we copied data there
    movl boot_info_copy, %eax
    testl %eax, %eax
    jz .no_param
    movabsq $boot_info_copy, %rdi
    jmp .have_param

.no_param:
    xorq %rdi, %rdi

.have_param:
    # Save boot parameter
    pushq %rdi

    # Clear BSS section
    movq $__bss, %rdi
    movq $__bss_end, %rcx
    subq %rdi, %rcx
    xorq %rax, %rax
    rep stosb

    # Restore boot parameter and pass as first argument to kernel_main (in %rdi)
    popq %rdi
    call kernel_main

    # Should never return, but if it does, halt
1:  hlt
    jmp 1b

