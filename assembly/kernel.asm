
org 0x0000
bits 16

; ========================================
; STAGE 1: SPLASH SCREEN
; ========================================
start:
    cli
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Set VGA mode 13h
    mov ax, 0x0013
    int 0x10

    ; Show splash
    call draw_splash_improved
    
    ; Transition to protected mode
    jmp enter_protected_mode

; ========================================
; IMPROVED SPLASH - Animated with progress
; ========================================
draw_splash_improved:
    push ax
    push bx
    push cx
    push dx
    push es

    mov ax, 0xA000
    mov es, ax

    ; Clear to black
    xor di, di
    xor al, al
    mov cx, 64000
    rep stosb

    ; Draw gradient background (top to bottom)
    mov dx, 0
.gradient_loop:
    mov di, dx
    mov ax, 320
    mul di
    mov di, ax
    
    ; Calculate color based on Y
    mov al, dl
    shr al, 4
    add al, 1
    
    mov cx, 320
    rep stosb
    
    inc dx
    cmp dx, 200
    jl .gradient_loop

    ; Draw "BUCKET OS" with shadow effect
    ; Shadow
    mov si, splash_text
    mov bx, 121
    mov dx, 91
    mov ah, 8  ; Dark gray
    call draw_text_rm
    
    ; Main text
    mov si, splash_text
    mov bx, 120
    mov dx, 90
    mov ah, 15  ; White
    call draw_text_rm

    ; Draw version
    mov si, version_text
    mov bx, 110
    mov dx, 105
    mov ah, 7
    call draw_text_rm

    ; Animated loading with progress
    call draw_loading_animated

    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret

splash_text: db 'BUCKET OS', 0
version_text: db 'Version 1.0', 0

; ========================================
; ANIMATED LOADING BAR
; ========================================
draw_loading_animated:
    push ax
    push bx
    push cx
    push dx

    ; Draw loading bar background
    mov bx, 80
    mov dx, 120
    mov cx, 160
    mov al, 8
    call draw_box_rm

    ; Draw loading bar frame
    mov bx, 80
    mov dx, 120
    mov cx, 160
    mov si, 10
    mov al, 15
    call draw_frame_rm

    ; Animate loading
    mov cx, 156
.load_loop:
    push cx
    
    ; Draw progress bar segment
    mov ax, 156
    sub ax, cx
    add ax, 82
    mov bx, ax
    mov dx, 122
    
    mov di, dx
    mov ax, 320
    mul di
    add ax, bx
    mov di, ax
    
    ; Gradient color based on position
    mov ax, 156
    sub ax, cx
    shr ax, 3
    add al, 10
    
    mov cx, 6
.fill_col:
    mov [es:di], al
    add di, 320
    loop .fill_col
    
    ; Delay
    push cx
    mov cx, 0x600
.delay:
    nop
    loop .delay
    pop cx
    
    pop cx
    loop .load_loop

    ; Final pause
    mov cx, 0x8000
.wait:
    loop .wait

    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ========================================
; DRAW TEXT (Real Mode)
; ========================================
draw_text_rm:
    push ax
    push bx
    push cx
    push dx
    push si
    
.next_char:
    lodsb
    test al, al
    jz .done
    
    cmp al, ' '
    je .space
    
    push si
    call draw_char_rm
    pop si
    
.space:
    add bx, 8
    jmp .next_char

.done:
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ========================================
; DRAW CHARACTER (Real Mode)
; ========================================
draw_char_rm:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    
    sub al, 32
    and ax, 0xFF
    shl ax, 3
    mov si, ax
    add si, font_data_rm
    
    mov cx, 8
.row:
    push cx
    push bx
    lodsb
    mov cl, 8
.col:
    shl al, 1
    jnc .skip
    
    push ax
    mov ax, dx
    mov di, 320
    mul di
    add ax, bx
    mov di, ax
    mov al, ah  ; Get color from AH register
    mov [es:di], al
    pop ax
    
.skip:
    inc bx
    dec cl
    jnz .col
    
    pop bx
    inc dx
    pop cx
    loop .row
    
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ========================================
; DRAW BOX (Real Mode)
; ========================================
draw_box_rm:
    push ax
    push bx
    push cx
    push dx
    push di
    
    mov si, 10
.row:
    push cx
    push bx
    
    mov ax, dx
    mov di, 320
    mul di
    add ax, bx
    mov di, ax
    
.col:
    stosb
    loop .col
    
    pop bx
    pop cx
    inc dx
    dec si
    jnz .row
    
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ========================================
; DRAW FRAME (Real Mode)
; ========================================
draw_frame_rm:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    
    ; Top line
    push si
    mov si, 1
    call draw_box_rm
    pop si
    
    ; Bottom line
    push dx
    add dx, si
    dec dx
    push si
    mov si, 1
    call draw_box_rm
    pop si
    pop dx
    
    ; Left line
    push cx
    mov cx, 1
    call draw_box_rm
    pop cx
    
    ; Right line
    push bx
    add bx, cx
    dec bx
    push cx
    mov cx, 1
    call draw_box_rm
    pop cx
    pop bx
    
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ========================================
; ENTER PROTECTED MODE
; ========================================
enter_protected_mode:
    cli

    ; Enable A20 (robust method)
    call enable_a20_robust

    ; Load GDT
    lgdt [gdt_descriptor]

    ; Switch to protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to protected mode
    db 0x66
    db 0xEA
    dd 0x10000 + protected_mode_start
    dw 0x08

; ========================================
; ROBUST A20 ENABLE
; ========================================
enable_a20_robust:
    ; Try fast method first
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Verify by testing
    call test_a20
    cmp ax, 1
    je .done
    
    ; Try keyboard controller method
    call enable_a20_kbd
    
.done:
    ret

test_a20:
    push es
    push ds
    
    xor ax, ax
    mov es, ax
    mov di, 0x0500
    
    mov ax, 0xFFFF
    mov ds, ax
    mov si, 0x0510
    
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
    
    mov al, [es:di]
    
    pop ds
    pop es
    
    cmp al, 0xFF
    je .disabled
    mov ax, 1
    ret
.disabled:
    xor ax, ax
    ret

enable_a20_kbd:
    cli
    
    call kbd_wait
    mov al, 0xAD
    out 0x64, al
    
    call kbd_wait
    mov al, 0xD0
    out 0x64, al
    
    call kbd_wait2
    in al, 0x60
    push ax
    
    call kbd_wait
    mov al, 0xD1
    out 0x64, al
    
    call kbd_wait
    pop ax
    or al, 2
    out 0x60, al
    
    call kbd_wait
    mov al, 0xAE
    out 0x64, al
    
    call kbd_wait
    sti
    ret

kbd_wait:
    in al, 0x64
    test al, 2
    jnz kbd_wait
    ret

kbd_wait2:
    in al, 0x64
    test al, 1
    jz kbd_wait2
    ret

; ========================================
; GDT
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
; PROTECTED MODE - IMPROVED GUI
; ========================================
bits 32
protected_mode_start:
    ; Setup segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Initialize improved system
    call init_improved_system
    call setup_ps2_mouse_driver
    call remap_pic

    ; Enable interrupts (we'll poll for now)
    sti

    ; Main loop with double buffering
main_loop:
    call read_ps2_mouse
    call process_keyboard_buffer
    call update_window_manager
    call render_to_backbuffer
    call flip_backbuffer
    
    ; 60 FPS target
    call vsync_wait
    
    jmp main_loop

; ========================================
; INIT IMPROVED SYSTEM
; ========================================
init_improved_system:
    ; Clear all buffers
    mov edi, backbuffer
    xor eax, eax
    mov ecx, 16000
    rep stosd
    
    mov edi, 0xA0000
    xor eax, eax
    mov ecx, 16000
    rep stosd

    ; Initialize mouse state
    mov dword [mouse_x], 160
    mov dword [mouse_y], 100
    mov byte [mouse_buttons], 0
    mov byte [mouse_button_prev], 0
    mov byte [mouse_packet_index], 0
    
    ; Initialize keyboard buffer
    mov byte [kbd_buffer_head], 0
    mov byte [kbd_buffer_tail], 0
    
    ; Initialize UI state
    mov byte [start_menu_open], 0
    mov dword [window_count], 0
    mov dword [active_window], 0xFFFFFFFF
    mov byte [dragging_window], 0
    
    ; Draw initial frame
    call draw_desktop_improved
    call draw_taskbar_improved
    call draw_desktop_icons_improved
    
    ret

; ========================================
; REMAP PIC
; ========================================
remap_pic:
    ; Save masks
    in al, 0x21
    mov [.pic1_mask], al
    in al, 0xA1
    mov [.pic2_mask], al
    
    ; Initialize PICs
    mov al, 0x11
    out 0x20, al
    call io_wait
    out 0xA0, al
    call io_wait
    
    ; Set IRQ offsets
    mov al, 0x20
    out 0x21, al
    call io_wait
    mov al, 0x28
    out 0xA1, al
    call io_wait
    
    ; Set cascade
    mov al, 0x04
    out 0x21, al
    call io_wait
    mov al, 0x02
    out 0xA1, al
    call io_wait
    
    ; Set 8086 mode
    mov al, 0x01
    out 0x21, al
    call io_wait
    out 0xA1, al
    call io_wait
    
    ; Restore masks
    mov al, [.pic1_mask]
    out 0x21, al
    mov al, [.pic2_mask]
    out 0xA1, al
    
    ret

.pic1_mask: db 0
.pic2_mask: db 0

io_wait:
    push ax
    xor al, al
    out 0x80, al
    pop ax
    ret

; ========================================
; PS/2 MOUSE DRIVER (Proper 3-byte packets)
; ========================================
setup_ps2_mouse_driver:
    ; Enable auxiliary device
    call mouse_wait_write
    mov al, 0xA8
    out 0x64, al
    
    ; Get compaq status
    call mouse_wait_write
    mov al, 0x20
    out 0x64, al
    call mouse_wait_read
    in al, 0x60
    or al, 2
    push eax
    
    ; Set compaq status
    call mouse_wait_write
    mov al, 0x60
    out 0x64, al
    call mouse_wait_write
    pop eax
    out 0x60, al
    
    ; Set defaults
    call mouse_write_cmd
    mov al, 0xF6
    out 0x60, al
    call mouse_wait_ack
    
    ; Enable data reporting
    call mouse_write_cmd
    mov al, 0xF4
    out 0x60, al
    call mouse_wait_ack
    
    ret

mouse_wait_write:
    push eax
.loop:
    in al, 0x64
    test al, 2
    jnz .loop
    pop eax
    ret

mouse_wait_read:
    push eax
.loop:
    in al, 0x64
    test al, 1
    jz .loop
    pop eax
    ret

mouse_write_cmd:
    call mouse_wait_write
    mov al, 0xD4
    out 0x64, al
    call mouse_wait_write
    ret

mouse_wait_ack:
    call mouse_wait_read
    in al, 0x60
    ret

; ========================================
; READ PS/2 MOUSE (3-byte packet)
; ========================================
read_ps2_mouse:
    ; Check if data available
    in al, 0x64
    test al, 0x20
    jz .no_data
    
    ; Read byte
    in al, 0x60
    
    ; Store in packet buffer
    movzx ebx, byte [mouse_packet_index]
    mov [mouse_packet_buffer + ebx], al
    inc byte [mouse_packet_index]
    
    ; Check if we have complete packet
    cmp byte [mouse_packet_index], 3
    jl .no_data
    
    ; Reset index
    mov byte [mouse_packet_index], 0
    
    ; Process packet
    call process_mouse_packet
    
.no_data:
    ret

; ========================================
; PROCESS MOUSE PACKET
; ========================================
process_mouse_packet:
    ; Byte 0: Flags and button state
    mov al, [mouse_packet_buffer]
    
    ; Check if valid (bit 3 must be set)
    test al, 0x08
    jz .invalid
    
    ; Store previous button state
    mov al, [mouse_buttons]
    mov [mouse_button_prev], al
    
    ; Extract button state
    mov al, [mouse_packet_buffer]
    and al, 0x03  ; Left and right buttons
    mov [mouse_buttons], al
    
    ; Byte 1: X movement (signed)
    movsx eax, byte [mouse_packet_buffer + 1]
    add [mouse_x], eax
    
    ; Byte 2: Y movement (signed, inverted)
    movsx eax, byte [mouse_packet_buffer + 2]
    neg eax
    add [mouse_y], eax
    
    ; Clamp to screen
    cmp dword [mouse_x], 0
    jge .check_max_x
    mov dword [mouse_x], 0
.check_max_x:
    cmp dword [mouse_x], 312
    jle .check_min_y
    mov dword [mouse_x], 312
    
.check_min_y:
    cmp dword [mouse_y], 0
    jge .check_max_y
    mov dword [mouse_y], 0
.check_max_y:
    cmp dword [mouse_y], 192
    jle .invalid
    mov dword [mouse_y], 192
    
.invalid:
    ret

; ========================================
; PROCESS KEYBOARD BUFFER
; ========================================
process_keyboard_buffer:
    ; Check if keyboard data available
    in al, 0x64
    test al, 1
    jz .no_data
    
    ; Read scancode
    in al, 0x60
    mov [last_scancode], al
    
    ; Add to keyboard buffer
    movzx ebx, byte [kbd_buffer_tail]
    mov [kbd_buffer + ebx], al
    inc byte [kbd_buffer_tail]
    and byte [kbd_buffer_tail], 0x0F  ; Ring buffer
    
    ; Process scancodes
    call handle_keyboard_input
    
.no_data:
    ret

; ========================================
; HANDLE KEYBOARD INPUT
; ========================================
handle_keyboard_input:
    mov al, [last_scancode]
    
    ; ESC - Toggle start menu
    cmp al, 0x01
    je .toggle_menu
    
    ; Arrow keys (for testing without mouse)
    cmp al, 0x48  ; Up
    je .move_up
    cmp al, 0x50  ; Down
    je .move_down
    cmp al, 0x4B  ; Left
    je .move_left
    cmp al, 0x4D  ; Right
    je .move_right
    
    ; Enter - simulate click
    cmp al, 0x1C
    je .simulate_click
    
    ret

.toggle_menu:
    xor byte [start_menu_open], 1
    ret

.move_up:
    sub dword [mouse_y], 8
    cmp dword [mouse_y], 0
    jge .done
    mov dword [mouse_y], 0
    ret

.move_down:
    add dword [mouse_y], 8
    cmp dword [mouse_y], 192
    jle .done
    mov dword [mouse_y], 192
    ret

.move_left:
    sub dword [mouse_x], 8
    cmp dword [mouse_x], 0
    jge .done
    mov dword [mouse_x], 0
    ret

.move_right:
    add dword [mouse_x], 8
    cmp dword [mouse_x], 312
    jle .done
    mov dword [mouse_x], 312
    ret

.simulate_click:
    mov byte [mouse_buttons], 1
    call handle_mouse_click
    mov byte [mouse_buttons], 0
    ret

.done:
    ret

; ========================================
; UPDATE WINDOW MANAGER
; ========================================
update_window_manager:
    ; Detect click (button press with debouncing)
    mov al, [mouse_buttons]
    test al, 1
    jz .button_released
    
    ; Check if this is a new click
    mov al, [mouse_button_prev]
    test al, 1
    jnz .already_down
    
    ; New click!
    call handle_mouse_click
    ret

.already_down:
    ; Check if dragging window
    cmp byte [dragging_window], 1
    jne .done
    call drag_active_window
    ret

.button_released:
    mov byte [dragging_window], 0
    ret

.done:
    ret

; ========================================
; HANDLE MOUSE CLICK
; ========================================
handle_mouse_click:
    ; Check start button (2, 190, 60, 8)
    mov eax, [mouse_x]
    cmp eax, 2
    jl .not_start
    cmp eax, 62
    jg .not_start
    mov eax, [mouse_y]
    cmp eax, 190
    jl .not_start
    cmp eax, 198
    jg .not_start
    
    ; Toggle start menu
    xor byte [start_menu_open], 1
    ret

.not_start:
    ; Check window title bars (for dragging)
    call check_window_titlebar_click
    
    ; Check desktop icons
    call check_desktop_icon_click
    
    ret

; ========================================
; CHECK WINDOW TITLEBAR CLICK
; ========================================
check_window_titlebar_click:
    mov ecx, [window_count]
    dec ecx

.loop:
    cmp ecx, 0
    jl .done
    
    ; Get window
    mov eax, ecx
    imul eax, WINDOW_SIZE
    lea esi, [windows + eax]
    
    ; Check if visible
    cmp byte [esi + WIN_VISIBLE], 0
    je .next
    
    ; Check if mouse in title bar
    mov eax, [mouse_x]
    mov ebx, [mouse_y]
    
    cmp eax, [esi + WIN_X]
    jl .next
    mov edx, [esi + WIN_X]
    add edx, [esi + WIN_WIDTH]
    cmp eax, edx
    jg .next
    
    cmp ebx, [esi + WIN_Y]
    jl .next
    mov edx, [esi + WIN_Y]
    add edx, 12  ; Title bar height
    cmp ebx, edx
    jg .next
    
    ; Clicked on title bar!
    mov [active_window], ecx
    mov byte [dragging_window], 1
    
    ; Calculate drag offset
    mov eax, [mouse_x]
    sub eax, [esi + WIN_X]
    mov [drag_offset_x], eax
    
    mov eax, [mouse_y]
    sub eax, [esi + WIN_Y]
    mov [drag_offset_y], eax
    
    jmp .done

.next:
    dec ecx
    jmp .loop

.done:
    ret

; ========================================
; DRAG ACTIVE WINDOW
; ========================================
drag_active_window:
    cmp dword [active_window], 0xFFFFFFFF
    je .done
    
    mov eax, [active_window]
    imul eax, WINDOW_SIZE
    lea esi, [windows + eax]
    
    ; Calculate new position
    mov eax, [mouse_x]
    sub eax, [drag_offset_x]
    mov [esi + WIN_X], eax
    
    mov eax, [mouse_y]
    sub eax, [drag_offset_y]
    mov [esi + WIN_Y], eax
    
    ; Clamp to screen
    cmp dword [esi + WIN_X], 0
    jge .check_max_x
    mov dword [esi + WIN_X], 0

.check_max_x:
    mov eax, [esi + WIN_X]
    add eax, [esi + WIN_WIDTH]
    cmp eax, 320
    jle .check_min_y
    mov eax, 320
    sub eax, [esi + WIN_WIDTH]
    mov [esi + WIN_X], eax

.check_min_y:
    cmp dword [esi + WIN_Y], 0
    jge .check_max_y
    mov dword [esi + WIN_Y], 0

.check_max_y:
    mov eax, [esi + WIN_Y]
    add eax, [esi + WIN_HEIGHT]
    cmp eax, 190  ; Leave room for taskbar
    jle .done
    mov eax, 190
    sub eax, [esi + WIN_HEIGHT]
    mov [esi + WIN_Y], eax

.done:
    ret

; ========================================
; CHECK DESKTOP ICON CLICK
; ========================================
check_desktop_icon_click:
    mov ecx, 0

.loop:
    cmp ecx, 3
    jge .done
    
    ; Calculate icon position
    mov eax, ecx
    imul eax, 50
    add eax, 10
    
    ; Check bounds (10, Y, 40x40)
    mov ebx, [mouse_x]
    cmp ebx, 10
    jl .next
    cmp ebx, 50
    jg .next
    
    mov ebx, [mouse_y]
    cmp ebx, eax
    jl .next
    add eax, 40
    cmp ebx, eax
    jg .next
    
    ; Icon clicked! Open window
    push ecx
    call create_window_from_icon
    pop ecx
    jmp .done

.next:
    inc ecx
    jmp .loop

.done:
    ret

; ========================================
; CREATE WINDOW FROM ICON
; ECX = icon index
; ========================================
create_window_from_icon:
    ; Check window limit
    cmp dword [window_count], MAX_WINDOWS
    jge .no_space
    
    ; Get new window slot
    mov eax, [window_count]
    imul eax, WINDOW_SIZE
    lea edi, [windows + eax]
    
    ; Set base properties
    mov dword [edi + WIN_X], 80
    mov dword [edi + WIN_Y], 40
    mov dword [edi + WIN_WIDTH], 200
    mov dword [edi + WIN_HEIGHT], 120
    mov byte [edi + WIN_VISIBLE], 1
    mov byte [edi + WIN_MINIMIZED], 0
    
    ; Set properties based on icon
    cmp ecx, 0
    je .icon_computer
    cmp ecx, 1
    je .icon_files
    cmp ecx, 2
    je .icon_notes
    jmp .finish

.icon_computer:
    mov byte [edi + WIN_COLOR], 9  ; Blue
    lea esi, [title_computer]
    jmp .copy_title

.icon_files:
    mov byte [edi + WIN_COLOR], 14  ; Yellow
    lea esi, [title_files]
    jmp .copy_title

.icon_notes:
    mov byte [edi + WIN_COLOR], 15  ; White
    lea esi, [title_notes]

.copy_title:
    push edi
    lea edi, [edi + WIN_TITLE]
    mov ecx, 32
    rep movsb
    pop edi

.finish:
    inc dword [window_count]

.no_space:
    ret

; ========================================
; RENDER TO BACKBUFFER
; ========================================
render_to_backbuffer:
    ; Clear backbuffer
    mov edi, backbuffer
    xor eax, eax
    mov ecx, 16000
    rep stosd
    
    ; Draw all elements to backbuffer
    call draw_desktop_improved
    call draw_windows_improved
    call draw_taskbar_improved
    call draw_start_menu_improved
    call draw_desktop_icons_improved
    call draw_mouse_improved
    
    ret

; ========================================
; FLIP BACKBUFFER TO SCREEN
; ========================================
flip_backbuffer:
    push esi
    push edi
    push ecx
    
    mov esi, backbuffer
    mov edi, 0xA0000
    mov ecx, 16000
    rep movsd
    
    pop ecx
    pop edi
    pop esi
    ret

; ========================================
; VSYNC WAIT (Approximation)
; ========================================
vsync_wait:
    mov ecx, 0x8000
.wait:
    nop
    loop .wait
    ret

; ========================================
; DRAW DESKTOP (Improved gradient)
; ========================================
draw_desktop_improved:
    mov edi, backbuffer
    mov edx, 0
    
.row_loop:
    ; Calculate gradient color
    mov eax, edx
    shr eax, 4
    add al, 1
    
    mov ecx, 320
    rep stosb
    
    inc edx
    cmp edx, 190
    jl .row_loop
    
    ret

; ========================================
; DRAW TASKBAR (Improved with gradient)
; ========================================
draw_taskbar_improved:
    mov edi, backbuffer
    add edi, 60800
    
    ; Draw taskbar gradient
    mov edx, 0
.row:
    mov al, 8
    add al, dl
    mov ecx, 320
    rep stosb
    inc dl
    cmp dl, 10
    jl .row
    
    ; Draw start button with 3D effect
    mov ebx, 2
    mov edx, 190
    mov ecx, 60
    mov esi, 8
    mov al, 7
    call draw_button_3d
    
    ; Draw START text
    mov ebx, 16
    mov edx, 192
    mov esi, text_start
    mov al, 0
    call draw_text_pm
    
    ret

; ========================================
; DRAW 3D BUTTON
; ========================================
draw_button_3d:
    ; Draw base
    call draw_rect_pm
    
    ; Draw highlight (top-left)
    push eax
    mov al, 15
    push esi
    mov esi, 1
    call draw_rect_pm
    pop esi
    
    mov ebx, [esp + 16]
    mov edx, [esp + 12]
    mov ecx, 1
    call draw_rect_pm
    pop eax
    
    ; Draw shadow (bottom-right)
    push eax
    mov al, 0
    push ebx
    push edx
    add edx, esi
    dec edx
    push esi
    mov esi, 1
    call draw_rect_pm
    pop esi
    pop edx
    pop ebx
    
    add ebx, ecx
    dec ebx
    push ecx
    mov ecx, 1
    call draw_rect_pm
    pop ecx
    pop ebx
    pop eax
    
    ret

; ========================================
; DRAW START MENU (Improved)
; ========================================
draw_start_menu_improved:
    cmp byte [start_menu_open], 0
    je .closed
    
    ; Draw menu with shadow
    mov ebx, 4
    mov edx, 102
    mov ecx, 80
    mov esi, 85
    mov al, 0
    call draw_rect_pm
    
    mov ebx, 2
    mov edx, 100
    mov ecx, 80
    mov esi, 85
    mov al, 7
    call draw_rect_pm
    
    ; Draw border
    mov al, 15
    call draw_border_pm
    
    ; Draw menu items
    mov ebx, 10
    mov edx, 110
    mov esi, menu_item1
    mov al, 0
    call draw_text_pm
    
    add edx, 15
    mov esi, menu_item2
    call draw_text_pm
    
    add edx, 15
    mov esi, menu_item3
    call draw_text_pm
    
    add edx, 25
    mov esi, menu_item4
    call draw_text_pm
    
.closed:
    ret

; ========================================
; DRAW DESKTOP ICONS (Improved with icons)
; ========================================
draw_desktop_icons_improved:
    ; Icon 1: Computer with icon
    mov ebx, 10
    mov edx, 10
    call draw_computer_icon
    mov ebx, 12
    mov edx, 44
    mov esi, icon1_name
    mov al, 15
    call draw_text_pm
    
    ; Icon 2: Folder with icon
    mov ebx, 10
    mov edx, 60
    call draw_folder_icon
    mov ebx, 12
    mov edx, 94
    mov esi, icon2_name
    mov al, 15
    call draw_text_pm
    
    ; Icon 3: Document with icon
    mov ebx, 10
    mov edx, 110
    call draw_document_icon
    mov ebx, 12
    mov edx, 144
    mov esi, icon3_name
    mov al, 15
    call draw_text_pm
    
    ret

; ========================================
; DRAW ICON GRAPHICS
; ========================================
draw_computer_icon:
    ; Monitor body
    push ebx
    push edx
    add ebx, 6
    add edx, 4
    mov ecx, 20
    mov esi, 16
    mov al, 15
    call draw_rect_pm
    
    ; Screen
    add ebx, 2
    add edx, 2
    mov ecx, 16
    mov esi, 12
    mov al, 9
    call draw_rect_pm
    pop edx
    pop ebx
    
    ; Stand
    push ebx
    add ebx, 12
    add edx, 20
    mov ecx, 8
    mov esi, 8
    mov al, 15
    call draw_rect_pm
    pop ebx
    
    ret

draw_folder_icon:
    ; Folder body
    push ebx
    push edx
    add ebx, 4
    add edx, 10
    mov ecx, 24
    mov esi, 18
    mov al, 14
    call draw_rect_pm
    
    ; Folder tab
    sub edx, 6
    mov ecx, 12
    mov esi, 6
    call draw_rect_pm
    pop edx
    pop ebx
    
    ret

draw_document_icon:
    ; Document body
    push ebx
    push edx
    add ebx, 8
    add edx, 4
    mov ecx, 16
    mov esi, 24
    mov al, 15
    call draw_rect_pm
    
    ; Document lines
    add ebx, 2
    add edx, 6
    mov ecx, 12
    mov esi, 1
    mov al, 0
    call draw_rect_pm
    
    add edx, 4
    call draw_rect_pm
    
    add edx, 4
    call draw_rect_pm
    pop edx
    pop ebx
    
    ret

; ========================================
; DRAW WINDOWS (Improved)
; ========================================
draw_windows_improved:
    mov ecx, 0
    
.loop:
    cmp ecx, [window_count]
    jge .done
    
    push ecx
    mov eax, ecx
    imul eax, WINDOW_SIZE
    lea esi, [windows + eax]
    
    cmp byte [esi + WIN_VISIBLE], 0
    je .next
    
    cmp byte [esi + WIN_MINIMIZED], 1
    je .next
    
    ; Draw window shadow
    mov ebx, [esi + WIN_X]
    add ebx, 2
    mov edx, [esi + WIN_Y]
    add edx, 2
    mov ecx, [esi + WIN_WIDTH]
    push edx
    mov esi, [esi + WIN_HEIGHT]
    mov al, 0
    call draw_rect_pm
    
    pop edx
    pop ecx
    push ecx
    mov eax, ecx
    imul eax, WINDOW_SIZE
    lea esi, [windows + eax]
    
    ; Draw title bar
    mov ebx, [esi + WIN_X]
    mov edx, [esi + WIN_Y]
    mov ecx, [esi + WIN_WIDTH]
    push edx
    push ecx
    mov esi, 12
    mov al, 8
    call draw_rect_pm
    pop ecx
    pop edx
    
    ; Draw window body
    pop ecx
    push ecx
    mov eax, ecx
    imul eax, WINDOW_SIZE
    lea esi, [windows + eax]
    
    mov ebx, [esi + WIN_X]
    mov edx, [esi + WIN_Y]
    add edx, 12
    mov ecx, [esi + WIN_WIDTH]
    push esi
    mov esi, [esi + WIN_HEIGHT]
    sub esi, 12
    movzx eax, byte [esp]
    mov eax, [esp + 4]
    imul eax, WINDOW_SIZE
    movzx eax, byte [windows + eax + WIN_COLOR]
    call draw_rect_pm
    pop esi
    
    ; Draw border
    mov ebx, [esi + WIN_X]
    mov edx, [esi + WIN_Y]
    mov ecx, [esi + WIN_WIDTH]
    push esi
    mov esi, [esi + WIN_HEIGHT]
    mov al, 15
    call draw_border_pm
    pop esi
    
    ; Draw close button
    mov ebx, [esi + WIN_X]
    add ebx, [esi + WIN_WIDTH]
    sub ebx, 14
    mov edx, [esi + WIN_Y]
    add edx, 2
    mov ecx, 10
    mov esi, 8
    mov al, 12
    call draw_button_3d
    
    ; Draw X
    pop ecx
    push ecx
    mov eax, ecx
    imul eax, WINDOW_SIZE
    lea esi, [windows + eax]
    
    mov ebx, [esi + WIN_X]
    add ebx, [esi + WIN_WIDTH]
    sub ebx, 11
    mov edx, [esi + WIN_Y]
    add edx, 5
    mov esi, text_close
    mov al, 15
    call draw_text_pm
    
.next:
    pop ecx
    inc ecx
    jmp .loop

.done:
    ret

; ========================================
; DRAW MOUSE (Improved with shadow)
; ========================================
draw_mouse_improved:
    ; Draw shadow
    mov esi, 0
.shadow_row:
    mov edi, 0
.shadow_col:
    mov eax, esi
    shl eax, 3
    add eax, edi
    
    movzx ebx, byte [cursor_data + eax]
    cmp bl, 0
    je .skip_shadow
    
    mov eax, [mouse_y]
    add eax, esi
    add eax, 1
    cmp eax, 200
    jge .skip_shadow
    imul eax, 320
    push ebx
    mov ebx, [mouse_x]
    add ebx, edi
    add ebx, 1
    cmp ebx, 320
    jge .skip_shadow_pop
    add eax, ebx
    add eax, backbuffer
    pop ebx
    
    mov byte [eax], 0
    
.skip_shadow:
    inc edi
    cmp edi, 8
    jl .shadow_col
    inc esi
    cmp esi, 8
    jl .shadow_row
    
    ; Draw cursor
    mov esi, 0
.row_loop:
    mov edi, 0
.col_loop:
    mov eax, esi
    shl eax, 3
    add eax, edi
    
    movzx ebx, byte [cursor_data + eax]
    cmp bl, 0
    je .skip
    
    mov eax, [mouse_y]
    add eax, esi
    cmp eax, 200
    jge .skip
    imul eax, 320
    push ebx
    mov ebx, [mouse_x]
    add ebx, edi
    cmp ebx, 320
    jge .skip_pop
    add eax, ebx
    add eax, backbuffer
    pop ebx
    
    mov byte [eax], bl
    
.skip:
    inc edi
    cmp edi, 8
    jl .col_loop
    inc esi
    cmp esi, 8
    jl .row_loop
    ret

.skip_pop:
    pop ebx
    jmp .skip

.skip_shadow_pop:
    pop ebx
    jmp .skip_shadow

; ========================================
; DRAWING PRIMITIVES (Protected Mode)
; ========================================

draw_rect_pm:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    mov edi, backbuffer
    
.row:
    push ecx
    push ebx
    
    mov eax, edx
    imul eax, 320
    add eax, ebx
    add edi, eax
    
    mov al, [esp + 24]
.col:
    stosb
    loop .col
    
    sub edi, eax
    
    pop ebx
    pop ecx
    inc edx
    dec esi
    jnz .row
    
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

draw_border_pm:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    
    ; Top
    push esi
    mov esi, 1
    call draw_rect_pm
    pop esi
    
    ; Bottom
    push edx
    add edx, esi
    dec edx
    push esi
    mov esi, 1
    call draw_rect_pm
    pop esi
    pop edx
    
    ; Left
    push ecx
    mov ecx, 1
    call draw_rect_pm
    pop ecx
    
    ; Right
    push ebx
    add ebx, ecx
    dec ebx
    push ecx
    mov ecx, 1
    call draw_rect_pm
    pop ecx
    pop ebx
    
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

draw_text_pm:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    mov [.color], al
    
.next:
    lodsb
    test al, al
    jz .done
    
    cmp al, 32
    jl .next
    
    push esi
    call draw_char_pm
    pop esi
    
    add ebx, 8
    jmp .next
    
.done:
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

.color: db 0

draw_char_pm:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    sub al, 32
    and eax, 0xFF
    shl eax, 3
    lea esi, [font_data_pm + eax]
    
    xor edi, edi
.row:
    lodsb
    mov cl, 8
    
.bit:
    shl al, 1
    jnc .skip
    
    push eax
    push ebx
    
    mov eax, edx
    add eax, edi
    imul eax, 320
    add eax, ebx
    add eax, backbuffer
    
    mov bl, [draw_text_pm.color]
    mov byte [eax], bl
    
    pop ebx
    pop eax
    
.skip:
    inc ebx
    dec cl
    jnz .bit
    
    sub ebx, 8
    inc edi
    cmp edi, 8
    jl .row
    
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

; ========================================
; DATA SECTION
; ========================================

; Mouse state
mouse_x: dd 160
mouse_y: dd 100
mouse_buttons: db 0
mouse_button_prev: db 0
mouse_packet_buffer: times 3 db 0
mouse_packet_index: db 0

; Keyboard buffer
kbd_buffer: times 16 db 0
kbd_buffer_head: db 0
kbd_buffer_tail: db 0
last_scancode: db 0

; UI state
start_menu_open: db 0
window_count: dd 0
MAX_WINDOWS equ 10
active_window: dd 0xFFFFFFFF
dragging_window: db 0
drag_offset_x: dd 0
drag_offset_y: dd 0

; Window structure
WINDOW_SIZE equ 256
WIN_X equ 0
WIN_Y equ 4
WIN_WIDTH equ 8
WIN_HEIGHT equ 12
WIN_VISIBLE equ 16
WIN_MINIMIZED equ 17
WIN_COLOR equ 18
WIN_TITLE equ 32

; Window array
windows: times (MAX_WINDOWS * WINDOW_SIZE) db 0

; Text strings
text_start: db 'START', 0
text_close: db 'X', 0
menu_item1: db 'Programs', 0
menu_item2: db 'Settings', 0
menu_item3: db 'About', 0
menu_item4: db 'Shutdown', 0
icon1_name: db 'My PC', 0
icon2_name: db 'Files', 0
icon3_name: db 'Notes', 0
title_computer: db 'My Computer', 0, times (32-12) db 0
title_files: db 'File Explorer', 0, times (32-14) db 0
title_notes: db 'Notepad', 0, times (32-8) db 0

; Cursor data
cursor_data:
    db 15,15,0,0,0,0,0,0
    db 15,15,15,0,0,0,0,0
    db 15,15,15,15,0,0,0,0
    db 15,15,15,15,15,0,0,0
    db 15,15,15,15,15,15,0,0
    db 15,15,15,15,15,0,0,0
    db 15,15,15,15,0,0,0,0
    db 15,15,0,0,0,0,0,0

; Font data (Real Mode)
font_data_rm:
    times 256 db 0x00
    times 256 db 0x00
    db 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  ; Space
    times 120 db 0x00
    db 0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00  ; 0
    db 0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00  ; 1
    db 0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00  ; 2
    db 0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00  ; 3
    db 0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00  ; 4
    db 0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00  ; 5
    db 0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00  ; 6
    db 0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00  ; 7
    db 0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00  ; 8
    db 0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00  ; 9
    times 56 db 0x00
    db 0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00  ; A
    db 0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00  ; B
    db 0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00  ; C
    db 0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00  ; D
    db 0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00  ; E
    db 0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00  ; F
    db 0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00  ; G
    db 0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00  ; H
    db 0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00  ; I
    db 0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00  ; J
    db 0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00  ; K
    db 0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00  ; L
    db 0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00  ; M
    db 0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00  ; N
    db 0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00  ; O
    db 0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00  ; P
    db 0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00  ; Q
    db 0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00  ; R
    db 0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00  ; S
    db 0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00  ; T
    db 0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00  ; U
    db 0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00  ; V
    db 0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00  ; W
    db 0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00  ; X
    db 0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00  ; Y
    db 0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00  ; Z

; Font data (Protected Mode - same as real mode)
font_data_pm:
    times 256 db 0x00
    times 256 db 0x00
    db 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    times 120 db 0x00
    db 0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00
    db 0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00
    db 0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00
    db 0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00
    db 0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00
    db 0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00
    db 0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00
    db 0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00
    db 0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00
    db 0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00
    times 56 db 0x00
    db 0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00
    db 0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00
    db 0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00
    db 0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00
    db 0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00
    db 0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00
    db 0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00
    db 0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00
    db 0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00
    db 0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00
    db 0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00
    db 0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00
    db 0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00
    db 0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00
    db 0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00
    db 0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00
    db 0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00
    db 0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00
    db 0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00
    db 0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00
    db 0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00
    db 0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00
    db 0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00
    db 0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00
    db 0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00
    db 0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00

; Backbuffer for double buffering (64000 bytes)
align 4
backbuffer: times 64000 db 0
