OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")

/* Disk boot linker script - kernel loaded at 0x8000 */
SECTIONS {
    /* Bootloader section for disk boot (0x7C00) */
    .bootloader 0x7C00 : {
        KEEP(*(.bootloader))
    }

    /* Multiboot header must be early in the file */
    .multiboot : {
        KEEP(*(.multiboot))
    }

    /* Set base address for disk boot */
    . = 0x8000;
    __kernel_start = .;

    .text : {
        /* Disk boot 16-bit entry point */
        KEEP(*(.text.boot16))
        /* Regular boot entry */
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    .rodata : ALIGN(8) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(8) {
        *(.data .data.*)
    }

    /* Place BSS above 4MB to avoid both the VGA/ROM region (0xA0000-0xFFFFF)
     * and the cmdline at 0x200000 (injected by QEMU loader for disk boot).
     * The disk boot kernel loads at 0x8000 and large driver RX/TX buffers
     * in BSS can grow past 0xA0000 (VGA RAM) where writes are discarded.
     * 0x400000 is safe: identity-mapped, above cmdline, well into extended memory. */
    . = 0x400000;
    .bss : {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(4096);
    __stack_bottom = .;
    . += 128 * 1024; /* 128KB stack */
    __stack_top = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}