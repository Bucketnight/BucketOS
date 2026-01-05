; ========================================
; BOOT.ASM - Fixed Bootloader
; Loads kernel at 0x10000 and jumps directly
; No relocation needed
; ========================================

[BITS 16]
[ORG 0x7C00]

start:
    ; Save boot drive
    mov [boot_drive], dl
    
    ; Setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    
    ; Set VGA mode 13h
    mov ax, 0x0013
    int 0x10
    
    ; Display loading message
    mov si, msg_loading
    call print_string
    
    ; Load kernel from disk to 0x10000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    
    mov ah, 0x02        ; Read sectors
    mov al, 100         ; Read 100 sectors (50KB) - enough for C kernel
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start from sector 2
    mov dh, 0           ; Head 0
    mov dl, [boot_drive]
    
    int 0x13
    jc disk_error
    
    ; Display success
    mov si, msg_success
    call print_string
    
    ; Enable A20 line
    call enable_a20
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to protected mode
    jmp 0x08:protected_mode

disk_error:
    mov si, msg_error
    call print_string
    jmp $

; ========================================
; PRINT STRING (Real Mode)
; ========================================
print_string:
    pusha
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

; ========================================
; ENABLE A20 LINE
; ========================================
enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; ========================================
; GDT
; ========================================
align 8
gdt_start:
    ; Null descriptor
    dq 0x0000000000000000
    
    ; Code segment (0x08) - base 0, limit 4GB
    dw 0xFFFF       ; Limit (low)
    dw 0x0000       ; Base (low)
    db 0x00         ; Base (mid)
    db 10011010b    ; Access: present, ring 0, code, executable, readable
    db 11001111b    ; Flags: 4KB granularity, 32-bit + Limit (high)
    db 0x00         ; Base (high)
    
    ; Data segment (0x10) - base 0, limit 4GB
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b    ; Access: present, ring 0, data, writable
    db 11001111b
    db 0x00

gdt_descriptor:
    dw gdt_descriptor - gdt_start - 1
    dd gdt_start

; ========================================
; PROTECTED MODE SETUP
; ========================================
[BITS 32]
protected_mode:
    ; Setup segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000    ; Stack at 640KB - 64KB
    
    ; Jump to C kernel (loaded at 0x10000)
    ; The linker script will place _start at the beginning
    jmp 0x10000

; ========================================
; DATA
; ========================================
boot_drive: db 0
msg_loading: db 'Loading Bucket OS...', 0x0D, 0x0A, 0
msg_success: db 'Kernel loaded!', 0x0D, 0x0A, 0
msg_error: db 'Disk error!', 0x0D, 0x0A, 0

; ========================================
; BOOT SIGNATURE
; ========================================
times 510-($-$$) db 0
dw 0xAA55