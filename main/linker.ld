/* Linker script for Bucket OS C Kernel */

OUTPUT_FORMAT("binary")
ENTRY(_start)

SECTIONS
{
    /* Kernel loaded at 64KB (0x10000) */
    . = 0x10000;
    
    .text : ALIGN(4K) {
    *(.text.entry)
    *(.text*)
}

.rodata : ALIGN(4K) {
    *(.rodata*)
}

.data : ALIGN(4K) {
    *(.data*)
}

.bss : ALIGN(4K) {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
}

    
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note*)
    }
}