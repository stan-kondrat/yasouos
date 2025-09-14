ENTRY(_start)
OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")

SECTIONS {
    . = 0x100000;  /* 1MB - typical kernel load address for x86-64 */

    .multiboot : {
        KEEP(*(.multiboot))
    }

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
}