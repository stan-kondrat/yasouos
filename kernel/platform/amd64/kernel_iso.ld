OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")

/* ISO boot linker script - all code in .text section */
SECTIONS {
    /* Bootloader runs at 0x7C00 when loaded by El Torito */
    . = 0x7C00;
    __kernel_start = .;

    .text : {
        /* ISO bootloader entry (_start) */
        KEEP(*(.text.start))
        /* Disk boot 16-bit entry point (mode transitions) */
        KEEP(*(.text.boot16))
        /* Regular boot entry */
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    .rodata : ALIGN(8) {
        *(.rodata .rodata.*)
        /* Kernel command line */
        *(.cmdline)
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

    /* Stack region */
    . = ALIGN(4096);
    __stack_bottom = .;
    . += 128 * 1024; /* 128KB stack */
    __stack_top = .;

    /* Discard debug sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.debug*)
    }
}
