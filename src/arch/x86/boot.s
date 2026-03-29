# boot.s: Multiboot header and early x86 entry stub (sets up stack, calls kernel_main).

# Reading guide:
# - Purpose: boot.s: Multiboot header and early x86 entry stub (sets up stack, calls kernel_main).
# - Start reading at: stack_bottom
# - Tip: This runs very early; assume interrupts off and a minimal environment.

.section .multiboot
.align 4
.long 0x1BADB002
.long 0x00000007
.long -(0x1BADB002 + 0x00000007)
.long 0x00000000
.long 0x00000000
.long 0x00000000
.long 0x00000000
.long 0x00000000
.long 0x00000000
.long 1024
.long 768
.long 32

.section .bss
.align 16
.global stack_bottom
.global stack_top
stack_bottom:
.skip 16384
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    cli
    mov $stack_top, %esp
    push %ebx
    push %eax
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
