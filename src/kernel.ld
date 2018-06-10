OUTPUT_ARCH("i386:x86-64")
ENTRY(start)

SECTIONS {
    . = 0x100000;
    .text : { *(.text) }
    .data : { *(.data, .rodata) }
    __bss_start = .;
    .bss : { *(.bss) }
    end = .; _end = .; __end = .;
}
