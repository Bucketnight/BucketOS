; ========================================
; BOOT SECTOR - Loads Protected Mode Kernel
; Assembles to exactly 512 bytes
; ========================================

[BITS 16]
[ORG 0x7C00]

; ========================================
; BOOT SECTOR START
; ========================================
start:
    ; Save boot drive number (BIOS passes in DL)
    mov [boot_drive], dl

    ; Disable interrupts during setup
    cli

    ; Clear segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; Set up stack (grows downward from 0x7C00)
    mov sp, 0x7C00

    ; Enable interrupts
    sti

    ; Print loading message
    mov si, msg_loading
    call print_string

; ========================================
; LOAD KERNEL FROM DISK
; ========================================
load_kernel:
    ; Reset disk system
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Load splash kernel only (sectors 2-10 at 0x1000:0x0000)
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov ah, 0x02        ; BIOS read sectors function
    mov al, 9           ; 9 sectors = 4.5KB (enough for splash)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start from sector 2
    mov dh, 0           ; Head 0
    mov dl, [boot_drive]

    int 0x13
    jc disk_error

    cmp al, 9           ; Verify all sectors loaded
    jne disk_error

    ; Print success message
    mov si, msg_success
    call print_string

    ; Jump to kernel
    jmp jump_to_kernel

; ========================================
; DISK ERROR HANDLER
; ========================================
disk_error:
    mov si, msg_error
    call print_string

    ; Infinite loop on error
    jmp $

; ========================================
; JUMP TO KERNEL
; ========================================
jump_to_kernel:
    ; Set up segments for kernel
    ; Kernel is loaded at 0x1000:0x0000
    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    ; Jump to kernel entry point
    ; Far jump to segment:offset
    jmp 0x1000:0x0000

; ========================================
; PRINT STRING FUNCTION
; SI = pointer to null-terminated string
; ========================================
print_string:
    pusha
    mov ah, 0x0E        ; BIOS teletype output

.loop:
    lodsb               ; Load byte from [SI] into AL, increment SI
    test al, al         ; Check for null terminator
    jz .done

    int 0x10            ; Print character
    jmp .loop

.done:
    popa
    ret

; ========================================
; DATA SECTION
; ========================================
boot_drive: db 0x00     ; Boot drive number (BIOS sets DL on boot)

msg_loading: db 'Loading kernel...', 0x0D, 0x0A, 0
msg_success: db 'Kernel loaded!', 0x0D, 0x0A, 0
msg_error:   db 'Disk read error!', 0x0D, 0x0A, 0

; ========================================
; PADDING AND BOOT SIGNATURE
; ========================================
; Fill rest of boot sector with zeros
times 510-($-$$) db 0

; Boot signature (must be at bytes 510-511)
dw 0xAA55
