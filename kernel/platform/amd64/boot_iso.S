# ISO Bootloader for YasouOS
# Boots via El Torito no-emulation mode
# Bootloader code at 0x7C00, kernel entry point (_start16) immediately follows

.section .text.start
.code16
.global _start

_start:
    cli

    # Clear segments
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    # Set stack
    movw $0x7C00, %sp

    # Store base address in BP
    movw $0x7C00, %bp

    # Print boot message
    leaw (msg_boot - _start)(%bp), %si
    call print_string

    # Enable A20 line
    inb $0x92, %al           # Read from port 0x92
    testb $2, %al            # Check if bit 1 (A20) is already set
    jnz .enable_a20_done     # If already enabled, skip write
    andb $0xFE, %al          # Clear bit 0 (fast reset) to avoid triggering reset
    orb $2, %al              # Set bit 1 (A20 enable)
    outb %al, $0x92          # Write back
.enable_a20_done:

    # Print starting message
    leaw (msg_success - _start)(%bp), %si
    call print_string

    # Jump to kernel entry point (_start16)
    # _start16 handles mode transitions and calls kernel_main
    # Note: linker places _start16 immediately after this bootloader code
    ljmp $0x0000, $_start16

halt:
    hlt
    jmp halt

# Shared print_string - used by both bootloader and kernel entry
print_string:
    lodsb
    orb %al, %al
    jz .done
    movb $0x0E, %ah
    movw $0x0007, %bx
    int $0x10
    jmp print_string
.done:
    ret

msg_boot:       .asciz "YasouOS ISO Bootloader\r\n"
msg_success:    .asciz "Starting kernel...\r\n"

# Kernel command line is defined in generated cmdline.S
# PVH boot info structure (ISO-only, passes cmdline to kernel)
# See xen/include/public/arch-x86/hvm/start_info.h
# Magic: 0x336ec578 ("xEn3")
.section .data
.global iso_boot_info
iso_boot_info:
    .long 0x336ec578          # magic
    .long 1                   # version
    .long 0                   # flags
    .long 0                   # nr_modules
    .quad 0                   # modlist_paddr
    .quad kernel_cmdline      # cmdline_paddr
    .quad 0                   # rsdp_paddr
    .quad 0                   # memmap_paddr
    .long 0                   # memmap_entries
    .long 0                   # reserved

# Kernel entry point at 0x7E00 - 16-bit to 64-bit transition
# This code runs after the bootloader at 0x7C00
.section .text.boot16
.code16
.global _start16

_start16:
    # We're now at the kernel entry point after copy
    cli

    # Set up segments
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x9000, %sp

    # Store base address in BP for PIC addressing
    movw $_start16, %bp

    # Print boot message in real mode
    leaw (msg_booting - _start16)(%bp), %si
    call print_string

    # Load GDT
    lgdt gdtptr

    # Switch to protected mode
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0

    # Far jump to 32-bit code
    ljmp $0x08, $protected_mode

msg_booting:
    .asciz "YasouOS booting...\r\n"

# GDT and pointers
.align 8
gdt:
    # NULL descriptor
    .quad 0

    # 32-bit Code segment (0x08)
    .word 0xFFFF      # limit low
    .word 0x0000      # base low
    .byte 0x00        # base middle
    .byte 0x9A        # access
    .byte 0xCF        # flags + limit high
    .byte 0x00        # base high

    # 32-bit Data segment (0x10)
    .word 0xFFFF      # limit low
    .word 0x0000      # base low
    .byte 0x00        # base middle
    .byte 0x92        # access
    .byte 0xCF        # flags + limit high
    .byte 0x00        # base high

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
gdt_end:

gdtptr:
    .word gdt_end - gdt - 1
    .long gdt

gdtptr64:
    .word gdt_end - gdt - 1
    .quad gdt

# 32-bit protected mode entry
.code32
protected_mode:
    # Set up protected mode segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Set up identity paging for first 4GB
    movl $0x1000, %edi
    movl %edi, %cr3
    xorl %eax, %eax
    movl $6144, %ecx            # Clear 24KB
    rep stosl
    movl %cr3, %edi

    # PML4[0] -> PDPT
    movl $0x2003, (%edi)
    addl $0x1000, %edi

    # PDPT entries
    movl $0x3003, (%edi)        # PDPT[0] -> PD0
    movl $0x4003, 8(%edi)       # PDPT[1] -> PD1
    movl $0x5003, 16(%edi)      # PDPT[2] -> PD2
    movl $0x6003, 24(%edi)      # PDPT[3] -> PD3

    # Fill PDs with 2MB page entries
    movl $0x3000, %edi          # Start at PD0
    movl $0x0083, %eax          # Present, R/W, 2MB page
    movl $2048, %ecx            # 2048 entries
.fill_pd:
    movl %eax, (%edi)
    addl $0x200000, %eax        # Next 2MB
    addl $8, %edi
    loop .fill_pd

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

    # Far jump to 64-bit code
    ljmp $0x18, $long_mode

# 64-bit long mode entry
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

    # Clear BSS section
    movq $__bss, %rdi
    movq $__bss_end, %rcx
    subq %rdi, %rcx
    xorq %rax, %rax
    rep stosb

    # Pass ISO boot info address to kernel_main (first argument in %rdi)
    movq $iso_boot_info, %rdi
    call kernel_main

    # Should never return
    cli
halt_loop:
    hlt
    jmp halt_loop
