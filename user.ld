OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x00400000;

    .text :
    {
        *(.text*)
        *(.rodata*)
        *(.data*)
    }

    .bss :
    {
        *(.bss*)
        *(COMMON)
    }
}
