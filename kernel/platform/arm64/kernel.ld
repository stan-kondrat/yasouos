ENTRY(_start)

SECTIONS {
    . = 0x40200000;  /* QEMU virt machine kernel load address for ARM64 (RAM base 0x40000000 + 2MB offset) */

    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4) {
        *(.data .data.*)
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*)
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024; /* 128KB stack */
    __stack_top = .;

    _end = .;
}