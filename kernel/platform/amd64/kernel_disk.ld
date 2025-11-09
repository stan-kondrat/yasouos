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

    .bss : ALIGN(8) {
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