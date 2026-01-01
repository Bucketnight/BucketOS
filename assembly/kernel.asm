; ========================================
; SPLASH SCREEN KERNEL
; Real Mode, VGA Mode 13h
; Loaded at 0x0000:0x0000 by bootloader
; ========================================
org 0x0000
bits 16

; ========================================
; ENTRY POINT
; ========================================
start:
    cli

    ; Set up segments (loaded at 0x1000:0x0000)
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00  ; Use safer stack location

    sti

    ; Set VGA mode 13h (320x200, 256 colors)
    mov ax, 0x0013
    int 0x10

    ; Clear screen to black
    call clear_screen

    ; Set up palette (simplified - no complex rainbow)
    call setup_palette

    ; Draw simple logo text
    call draw_logo_simple

    ; Draw simple loading bar
    call animate_loading_simple

    ; Jump to main kernel
    jmp main_kernel

; ========================================
; CLEAR SCREEN TO BLACK
; ========================================
clear_screen:
    push ax
    push cx
    push di
    push es

    mov ax, 0xA000
    mov es, ax
    xor di, di
    xor al, al
    mov cx, 64000
    rep stosb

    pop es
    pop di
    pop cx
    pop ax
    ret

; ========================================
; SETUP SIMPLE PALETTE
; ========================================
setup_palette:
    push ax
    push dx
    push cx

    ; Set basic 16 color palette
    mov dx, 0x03C8
    xor al, al
    out dx, al
    inc dx

    mov cx, 16
    xor al, al
.loop:
    ; Simple grayscale + colors
    out dx, al
    out dx, al
    out dx, al
    add al, 4
    loop .loop

    pop cx
    pop dx
    pop ax
    ret

; ========================================
; DRAW SIMPLE LOGO
; ========================================
draw_logo_simple:
    push ax
    push bx
    push cx
    push dx
    push si
    push es

    mov ax, 0xA000
    mov es, ax

    ; Draw "BUCKET OS" using BIOS font style
    mov si, logo_text
    mov bx, 110     ; X position (centered-ish)
    mov dx, 80      ; Y position

.next_char:
    lodsb
    test al, al
    jz .done

    cmp al, ' '
    je .space

    push si
    push bx
    push dx
    mov ah, 15      ; White color
    call draw_char_simple
    pop dx
    pop bx
    pop si

.space:
    add bx, 10
    jmp .next_char

.done:
    pop es
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

logo_text: db 'BUCKET OS', 0

; ========================================
; DRAW SIMPLE CHARACTER
; AL=char, BX=x, DX=y, AH=color
; ========================================
draw_char_simple:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es

    ; Get font data offset
    sub al, 32
    and ax, 0xFF
    mov cl, 3
    shl ax, cl
    mov si, ax
    add si, font_data

    ; Draw 8x8 character
    mov ax, 0xA000
    mov es, ax
    mov cx, 8

.row:
    push cx
    push bx

    lodsb
    mov cl, 8

.col:
    shl al, 1
    jnc .skip

    ; Draw pixel
    push ax
    mov ax, dx
    mov di, 320
    mul di
    add ax, bx
    mov di, ax
    mov al, [esp + 18]  ; Get color (AH from original call)
    stosb
    pop ax

.skip:
    inc bx
    dec cl
    jnz .col

    pop bx
    inc dx
    pop cx
    loop .row

    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ========================================
; ANIMATE SIMPLE LOADING BAR
; ========================================
animate_loading_simple:
    push ax
    push bx
    push cx
    push dx
    push es

    mov ax, 0xA000
    mov es, ax

    ; Draw loading bar frame
    mov bx, 80      ; X
    mov dx, 120     ; Y
    mov cx, 160     ; Width

    ; Top line
    push bx
    push dx
    mov di, dx
    mov ax, 320
    mul di
    add ax, bx
    mov di, ax
    mov al, 15
    mov cx, 160
    rep stosb
    pop dx
    pop bx

    ; Bottom line
    push bx
    add dx, 10
    mov di, dx
    mov ax, 320
    mul di
    add ax, bx
    mov di, ax
    mov al, 15
    mov cx, 160
    rep stosb
    pop bx

    ; Animate filling
    mov cx, 150
.fill:
    push cx
    push bx

    ; Draw one column
    mov dx, 122
    mov di, dx
    push ax
    mov ax, 320
    mul di
    mov di, ax
    pop ax
    add di, bx

    mov al, 10  ; Green
    mov cx, 8
.col:
    stosb
    add di, 319
    loop .col

    pop bx
    inc bx

    ; Delay
    push cx
    mov cx, 0x1000
.delay:
    nop
    loop .delay
    pop cx

    pop cx
    loop .fill

    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ========================================
; MAIN KERNEL (After splash)
; ========================================
main_kernel:
    ; Transition to protected mode and load init.asm
    cli

    ; Enable A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Load GDT
    lgdt [gdt_descriptor]

    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Jump to protected mode, then to init.asm
    db 0x66
    db 0xEA
    dd 0x10000 + protected_mode_start
    dw 0x08

; ========================================
; GDT (Global Descriptor Table)
; ========================================
align 8
gdt_start:
    dq 0x0000000000000000

    ; Code segment (0x08)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

    ; Data segment (0x10)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_descriptor:
    dw gdt_descriptor - gdt_start - 1
    dd 0x10000 + gdt_start

; ========================================
; PROTECTED MODE START
; ========================================
bits 32
protected_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Jump to init.asm (loaded at 0x20000)
    jmp 0x20000

; Pad to reasonable size
times 4096-($-$) db 0
