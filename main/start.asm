; ========================================
; START.ASM - Entry point for C kernel
; This avoids all GCC startup issues
; ========================================

[BITS 32]
[GLOBAL _start]
[EXTERN kernel_main]

section .text.entry

_start:
    ; Setup segments (already done by bootloader, but make sure)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Clear BSS section
    extern __bss_start
    extern __bss_end
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    
    ; Call C kernel_main
    call kernel_main
    
    ; If kernel_main returns, halt
.hang:
    cli
    hlt
    jmp .hang