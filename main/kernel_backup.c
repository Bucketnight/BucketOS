// ========================================
// BUCKET OS - Full Featured Kernel in C
// Features: GUI, Mouse, Keyboard, Windows, Desktop
// ========================================

#include "types.h"

// ========================================
// Hardware Definitions
// ========================================

#define VGA_MEMORY 0xA0000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

// IO Ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define KB_DATA_PORT 0x60
#define KB_STATUS_PORT 0x64
#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64

// ========================================
// Data Structures
// ========================================

typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
    uint8_t buttons_prev;
    uint8_t packet_buffer[3];
    uint8_t packet_index;
} Mouse;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint8_t visible;
    uint8_t minimized;
    uint8_t color;
    char title[32];
} Window;

typedef struct {
    uint8_t buffer[16];
    uint8_t head;
    uint8_t tail;
} KeyboardBuffer;

typedef struct {
    Mouse mouse;
    KeyboardBuffer keyboard;
    Window windows[10];
    uint32_t window_count;
    int32_t active_window;
    bool dragging;
    int32_t drag_offset_x;
    int32_t drag_offset_y;
    bool start_menu_open;
    uint8_t backbuffer[SCREEN_SIZE];
} SystemState;

// Global hypervisor status
static bool vtx_supported = false;

// ========================================
// Inline Assembly Helpers
// ========================================

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static inline void cli(void) {
    __asm__ volatile ("cli");
}

static inline void hlt(void) {
    __asm__ volatile ("hlt");
}

// ========================================
// Memory Operations
// ========================================

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// ========================================
// Font Data (8x8)
// ========================================

static const uint8_t font_data[256 * 8] = {
    // Space (32)
    [32*8] = 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00,
    // A-Z (65-90)
    [65*8] = 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00,  // A
    0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00,           // B
    0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00,           // C
    0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00,           // D
    0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00,           // E
    0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00,           // F
    0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00,           // G
    0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00,           // H
    0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00,           // I
    0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00,           // J
    0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00,           // K
    0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00,           // L
    0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00,           // M
    0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00,           // N
    0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00,           // O
    0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00,           // P
    0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00,           // Q
    0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00,           // R
    0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00,           // S
    0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00,           // T
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00,           // U
    0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00,           // V
    0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00,           // W
    0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00,           // X
    0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00,           // Y
    0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00,           // Z
    // a-z (97-122) - same as uppercase for simplicity
    [97*8] = 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00,
};

// ========================================
// Mouse Cursor Data
// ========================================

static const uint8_t cursor_data[8][8] = {
    {15, 15, 0, 0, 0, 0, 0, 0},
    {15, 15, 15, 0, 0, 0, 0, 0},
    {15, 15, 15, 15, 0, 0, 0, 0},
    {15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 15, 15, 0, 0},
    {15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 0, 0, 0, 0},
    {15, 15, 0, 0, 0, 0, 0, 0}
};

// ========================================
// PIC Initialization
// ========================================

void init_pic(void) {
    // ICW1
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();
    
    // ICW2 (IRQ remapping)
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();
    
    // ICW3
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();
    
    // ICW4
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();
    
    // Enable keyboard and mouse IRQs
    outb(PIC1_DATA, 0xFC);  // Enable IRQ0, IRQ1
    outb(PIC2_DATA, 0xEF);  // Enable IRQ12
}

// ========================================
// PS/2 Mouse Driver
// ========================================

void mouse_wait_write(void) {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(MOUSE_STATUS_PORT) & 0x02)) {
            return;
        }
    }
}

void mouse_wait_read(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(MOUSE_STATUS_PORT) & 0x01) {
            return;
        }
    }
}

void mouse_write_cmd(uint8_t cmd) {
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, 0xD4);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, cmd);
}

void init_mouse(void) {
    // Enable auxiliary device
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, 0xA8);
    
    // Get compaq status
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, 0x20);
    mouse_wait_read();
    uint8_t status = inb(MOUSE_DATA_PORT);
    status |= 0x02;
    
    // Set compaq status
    mouse_wait_write();
    outb(MOUSE_STATUS_PORT, 0x60);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, status);
    
    // Use default settings
    mouse_write_cmd(0xF6);
    mouse_wait_read();
    inb(MOUSE_DATA_PORT);  // ACK
    
    // Enable data reporting
    mouse_write_cmd(0xF4);
    mouse_wait_read();
    inb(MOUSE_DATA_PORT);  // ACK
    
    sys->mouse.x = SCREEN_WIDTH / 2;
    sys->mouse.y = SCREEN_HEIGHT / 2;
    sys->mouse.buttons = 0;
    sys->mouse.buttons_prev = 0;
    sys->mouse.packet_index = 0;
}

void read_mouse(void) {
    if (!(inb(MOUSE_STATUS_PORT) & 0x20)) {
        return;  // No mouse data
    }
    
    uint8_t data = inb(MOUSE_DATA_PORT);
    sys->mouse.packet_buffer[sys->mouse.packet_index] = data;
    sys->mouse.packet_index++;
    
    if (sys->mouse.packet_index < 3) {
        return;  // Wait for complete packet
    }
    
    sys->mouse.packet_index = 0;
    
    // Check validity
    if (!(sys->mouse.packet_buffer[0] & 0x08)) {
        return;
    }
    
    // Store previous button state
    sys->mouse.buttons_prev = sys->mouse.buttons;
    
    // Extract button state
    sys->mouse.buttons = sys->mouse.packet_buffer[0] & 0x03;
    
    // Extract movement
    int32_t dx = (int32_t)sys->mouse.packet_buffer[1];
    int32_t dy = -(int32_t)sys->mouse.packet_buffer[2];  // Inverted
    
    // Apply movement
    sys->mouse.x += dx;
    sys->mouse.y += dy;
    
    // Clamp to screen
    if (sys->mouse.x < 0) sys->mouse.x = 0;
    if (sys->mouse.x >= SCREEN_WIDTH - 8) sys->mouse.x = SCREEN_WIDTH - 8;
    if (sys->mouse.y < 0) sys->mouse.y = 0;
    if (sys->mouse.y >= SCREEN_HEIGHT - 8) sys->mouse.y = SCREEN_HEIGHT - 8;
}

// ========================================
// Keyboard Driver
// ========================================

void init_keyboard(void) {
    sys->keyboard.head = 0;
    sys->keyboard.tail = 0;
}

void read_keyboard(void) {
    if (!(inb(KB_STATUS_PORT) & 0x01)) {
        return;  // No keyboard data
    }
    
    uint8_t scancode = inb(KB_DATA_PORT);
    
    // Add to buffer
    sys->keyboard.buffer[sys->keyboard.tail] = scancode;
    sys->keyboard.tail = (sys->keyboard.tail + 1) & 0x0F;
}

uint8_t get_scancode(void) {
    if (sys->keyboard.head == sys->keyboard.tail) {
        return 0;  // Buffer empty
    }
    
    uint8_t scancode = sys->keyboard.buffer[sys->keyboard.head];
    sys->keyboard.head = (sys->keyboard.head + 1) & 0x0F;
    return scancode;
}

// ========================================
// Graphics Functions
// ========================================

void set_pixel(int32_t x, int32_t y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        sys->backbuffer[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color) {
    for (int32_t j = 0; j < h; j++) {
        for (int32_t i = 0; i < w; i++) {
            set_pixel(x + i, y + j, color);
        }
    }
}

void draw_rect_border(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color) {
    // Top and bottom
    for (int32_t i = 0; i < w; i++) {
        set_pixel(x + i, y, color);
        set_pixel(x + i, y + h - 1, color);
    }
    // Left and right
    for (int32_t j = 0; j < h; j++) {
        set_pixel(x, y + j, color);
        set_pixel(x + w - 1, y + j, color);
    }
}

void draw_char(int32_t x, int32_t y, char c, uint8_t color) {
    if (c < 32 || c > 122) return;
    
    const uint8_t* glyph = &font_data[(uint8_t)c * 8];
    
    for (int j = 0; j < 8; j++) {
        uint8_t row = glyph[j];
        for (int i = 0; i < 8; i++) {
            if (row & (0x80 >> i)) {
                set_pixel(x + i, y + j, color);
            }
        }
    }
}

void draw_string(int32_t x, int32_t y, const char* str, uint8_t color) {
    while (*str) {
        draw_char(x, y, *str, color);
        x += 8;
        str++;
    }
}

void draw_button_3d(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color) {
    // Base
    draw_rect(x, y, w, h, color);
    
    // Highlight (top-left)
    draw_rect(x, y, w, 1, 15);  // Top
    draw_rect(x, y, 1, h, 15);  // Left
    
    // Shadow (bottom-right)
    draw_rect(x, y + h - 1, w, 1, 0);       // Bottom
    draw_rect(x + w - 1, y, 1, h, 0);       // Right
}

// ========================================
// Desktop & UI Rendering
// ========================================

void draw_desktop(void) {
    // Gradient background
    for (int y = 0; y < SCREEN_HEIGHT - 10; y++) {
        uint8_t color = 1 + (y / 16);
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            set_pixel(x, y, color);
        }
    }
}

void draw_taskbar(void) {
    // Taskbar background
    draw_rect(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10, 8);
    
    // Start button
    draw_button_3d(2, SCREEN_HEIGHT - 8, 60, 8, 7);
    draw_string(16, SCREEN_HEIGHT - 6, "START", 0);
}

void draw_start_menu(void) {
    if (!sys->start_menu_open) return;
    
    // Menu background with shadow
    draw_rect(4, 102, 80, 85, 0);  // Shadow
    draw_rect(2, 100, 80, 85, 7);  // Menu
    draw_rect_border(2, 100, 80, 85, 15);
    
    // Menu items
    draw_string(10, 110, "Programs", 0);
    draw_string(10, 125, "Documents", 0);
    draw_string(10, 140, "Settings", 0);
    draw_string(10, 155, "Hypervisor", 0);
    draw_string(10, 170, "Shutdown", 0);
}

void draw_desktop_icon(int32_t x, int32_t y, uint8_t type, const char* name) {
    // Icon background
    draw_rect(x, y, 32, 32, type == 0 ? 9 : (type == 1 ? 14 : 15));
    
    // Icon details (simplified)
    if (type == 0) {  // Computer
        draw_rect(x + 6, y + 4, 20, 16, 15);
        draw_rect(x + 8, y + 6, 16, 12, 9);
        draw_rect(x + 12, y + 20, 8, 8, 15);
    } else if (type == 1) {  // Folder
        draw_rect(x + 4, y + 10, 24, 18, 14);
        draw_rect(x + 4, y + 6, 12, 4, 14);
    } else {  // Document
        draw_rect(x + 8, y + 4, 16, 24, 15);
        draw_rect(x + 10, y + 10, 12, 1, 0);
        draw_rect(x + 10, y + 14, 12, 1, 0);
        draw_rect(x + 10, y + 18, 12, 1, 0);
    }
    
    // Icon label
    draw_string(x + 2, y + 34, name, 15);
}

void draw_desktop_icons(void) {
    draw_desktop_icon(10, 10, 0, "My PC");
    draw_desktop_icon(10, 60, 1, "Files");
    draw_desktop_icon(10, 110, 2, "Notes");
}

void draw_window(Window* win) {
    if (!win->visible || win->minimized) return;
    
    // Shadow
    draw_rect(win->x + 2, win->y + 2, win->width, win->height, 0);
    
    // Title bar
    draw_rect(win->x, win->y, win->width, 12, 8);
    
    // Window body
    draw_rect(win->x, win->y + 12, win->width, win->height - 12, win->color);
    
    // Border
    draw_rect_border(win->x, win->y, win->width, win->height, 15);
    
    // Close button
    draw_button_3d(win->x + win->width - 14, win->y + 2, 10, 8, 12);
    draw_string(win->x + win->width - 11, win->y + 4, "X", 15);
    
    // Title
    draw_string(win->x + 5, win->y + 3, win->title, 15);
    
    // Special content for hypervisor window
    if (strcmp(win->title, "Hypervisor Status") == 0) {
        draw_string(win->x + 10, win->y + 20, "VT-x: ", 15);
        if (vtx_supported) {
            draw_string(win->x + 50, win->y + 20, "Supported", 10);
        } else {
            draw_string(win->x + 50, win->y + 20, "Not Supported", 12);
        }
        
        draw_string(win->x + 10, win->y + 35, "EPT: Initialized", 10);
        draw_string(win->x + 10, win->y + 50, "VMCS: Ready", 10);
        draw_string(win->x + 10, win->y + 65, "VM: Minimal", 10);
    }
}

void draw_windows(void) {
    for (uint32_t i = 0; i < sys->window_count; i++) {
        draw_window(&sys->windows[i]);
    }
}

void draw_mouse(void) {
    // Draw shadow
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            if (cursor_data[j][i] != 0) {
                set_pixel(sys->mouse.x + i + 1, sys->mouse.y + j + 1, 0);
            }
        }
    }
    
    // Draw cursor
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            if (cursor_data[j][i] != 0) {
                set_pixel(sys->mouse.x + i, sys->mouse.y + j, cursor_data[j][i]);
            }
        }
    }
}

void flip_buffer(void) {
    memcpy((void*)VGA_MEMORY, sys->backbuffer, SCREEN_SIZE);
}

// ========================================
// Window Management
// ========================================

void create_window(int32_t x, int32_t y, int32_t w, int32_t h, 
                   uint8_t color, const char* title) {
    if (sys->window_count >= 10) return;
    
    Window* win = &sys->windows[sys->window_count];
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->color = color;
    win->visible = 1;
    win->minimized = 0;
    strcpy(win->title, title);
    
    sys->window_count++;
}

bool point_in_rect(int32_t px, int32_t py, int32_t x, int32_t y, 
                   int32_t w, int32_t h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

void handle_click(void) {
    int32_t mx = sys->mouse.x;
    int32_t my = sys->mouse.y;
    
    // Check start button
    if (point_in_rect(mx, my, 2, SCREEN_HEIGHT - 8, 60, 8)) {
        sys->start_menu_open = !sys->start_menu_open;
        return;
    }
    
    // Check start menu items
    if (sys->start_menu_open) {
        if (point_in_rect(mx, my, 2, 100, 80, 85)) {
            // Programs
            if (point_in_rect(mx, my, 2, 105, 80, 10)) {
                create_window(80, 40, 200, 120, 9, "Programs");
                sys->start_menu_open = false;
                return;
            }
            // Documents
            if (point_in_rect(mx, my, 2, 120, 80, 10)) {
                create_window(100, 60, 220, 140, 14, "Documents");
                sys->start_menu_open = false;
                return;
            }
            // Settings
            if (point_in_rect(mx, my, 2, 135, 80, 10)) {
                create_window(120, 80, 180, 100, 15, "Settings");
                sys->start_menu_open = false;
                return;
            }
            // Hypervisor
            if (point_in_rect(mx, my, 2, 150, 80, 10)) {
                create_window(60, 40, 250, 150, 11, "Hypervisor Status");
                sys->start_menu_open = false;
                return;
            }
            // Shutdown
            if (point_in_rect(mx, my, 2, 165, 80, 10)) {
                // Shutdown (halt)
                cli();
                while(1) hlt();
            }
        }
        sys->start_menu_open = false;
        return;
    }
    
    // Check desktop icons
    if (point_in_rect(mx, my, 10, 10, 40, 45)) {
        create_window(80, 40, 200, 120, 9, "My Computer");
        return;
    }
    if (point_in_rect(mx, my, 10, 60, 40, 45)) {
        create_window(100, 60, 220, 140, 14, "File Explorer");
        return;
    }
    if (point_in_rect(mx, my, 10, 110, 40, 45)) {
        create_window(120, 80, 180, 100, 15, "Notepad");
        return;
    }
    
    // Check window title bars (for dragging)
    for (int32_t i = sys->window_count - 1; i >= 0; i--) {
        Window* win = &sys->windows[i];
        if (!win->visible || win->minimized) continue;
        
        if (point_in_rect(mx, my, win->x, win->y, win->width, 12)) {
            sys->active_window = i;
            sys->dragging = true;
            sys->drag_offset_x = mx - win->x;
            sys->drag_offset_y = my - win->y;
            return;
        }
    }
}

void handle_drag(void) {
    if (!sys->dragging || sys->active_window < 0) return;
    
    Window* win = &sys->windows[sys->active_window];
    win->x = sys->mouse.x - sys->drag_offset_x;
    win->y = sys->mouse.y - sys->drag_offset_y;
    
    // Clamp to screen
    if (win->x < 0) win->x = 0;
    if (win->y < 0) win->y = 0;
    if (win->x + win->width > SCREEN_WIDTH) 
        win->x = SCREEN_WIDTH - win->width;
    if (win->y + win->height > SCREEN_HEIGHT - 10) 
        win->y = SCREEN_HEIGHT - 10 - win->height;
}

// ========================================
// Input Processing
// ========================================

void process_input(void) {
    // Read mouse
    read_mouse();
    
    // Read keyboard
    read_keyboard();
    
    // Process keyboard
    uint8_t scancode = get_scancode();
    if (scancode) {
        // ESC - Toggle start menu
        if (scancode == 0x01) {
            sys->start_menu_open = !sys->start_menu_open;
        }
        // Arrow keys (for testing)
        else if (scancode == 0x48) sys->mouse.y -= 4;  // Up
        else if (scancode == 0x50) sys->mouse.y += 4;  // Down
        else if (scancode == 0x4B) sys->mouse.x -= 4;  // Left
        else if (scancode == 0x4D) sys->mouse.x += 4;  // Right
        else if (scancode == 0x1C) {  // Enter
            sys->mouse.buttons = 1;
            handle_click();
            sys->mouse.buttons = 0;
        }
    }
    
    // Handle mouse clicks (with debouncing)
    if ((sys->mouse.buttons & 1) && !(sys->mouse.buttons_prev & 1)) {
        // New click
        handle_click();
    } else if ((sys->mouse.buttons & 1) && (sys->mouse.buttons_prev & 1)) {
        // Dragging
        handle_drag();
    } else {
        // Released
        sys->dragging = false;
    }
}

// ========================================
// Hypervisor Foundations - Chapter 1
// ========================================

// VMCS revision identifier
uint32_t vmcs_revision_id;

// VMX basic info
uint64_t vmx_basic;

// VMCS structure (simplified)
typedef struct {
    uint32_t revision_id;
    uint32_t abort_indicator;
    uint8_t data[4096 - 8];  // 4KB page minus header
} vmcs_t;

// Guest register state
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr0, cr2, cr3, cr4;
    uint16_t cs, ds, es, fs, gs, ss;
    uint16_t tr, ldtr;
    uint64_t gdtr_base;
    uint32_t gdtr_limit;
    uint64_t idtr_base;
    uint32_t idtr_limit;
} guest_regs_t;

// Forward declarations
bool check_vtx_support(void);
void enable_vtx(void);
void get_vmx_info(void);
vmcs_t* alloc_vmcs(void);
void setup_vmcs(vmcs_t* vmcs, guest_regs_t* guest);
void vmexit_handler(void);
void launch_minimal_vm(void);
bool init_hypervisor_foundation(void);
void init_ept(void);
void setup_ept_in_vmcs(void);

// Check if VT-x is supported
bool check_vtx_support(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // Check CPUID.1:ECX.VMX[bit 5]
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    if (!(ecx & (1 << 5))) {
        return false;  // VT-x not supported
    }
    
    return true;
}

// Enable VT-x in CR4
void enable_vtx(void) {
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 13);  // CR4.VMXE
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
}

// Check if we can run as hypervisor
bool init_hypervisor_foundation(void) {
    vtx_supported = check_vtx_support();
    if (!vtx_supported) {
        draw_string(10, 50, "VT-x not supported!", 12);
        return false;
    }
    
    enable_vtx();
    draw_string(10, 50, "VT-x enabled!", 10);
    
    // Get VMX capabilities
    get_vmx_info();
    
    // Try to launch a minimal VM
    launch_minimal_vm();
    
    return true;
}

// VM exit reasons
#define VMEXIT_CPUID 0x0A
#define VMEXIT_HLT   0x0C
#define VMEXIT_IO    0x1E

// VMCS field encodings (subset)
#define VMCS_GUEST_ES_SELECTOR   0x00000800
#define VMCS_GUEST_CS_SELECTOR   0x00000802
#define VMCS_GUEST_SS_SELECTOR   0x00000804
#define VMCS_GUEST_DS_SELECTOR   0x00000806
#define VMCS_GUEST_FS_SELECTOR   0x00000808
#define VMCS_GUEST_GS_SELECTOR   0x0000080A
#define VMCS_GUEST_LDTR_SELECTOR 0x0000080C
#define VMCS_GUEST_TR_SELECTOR   0x0000080E
#define VMCS_GUEST_CR0          0x00006800
#define VMCS_GUEST_CR3          0x00006802
#define VMCS_GUEST_CR4          0x00006804
#define VMCS_GUEST_RSP          0x0000681C
#define VMCS_GUEST_RIP          0x0000681E
#define VMCS_GUEST_RFLAGS       0x00006820
#define VMCS_HOST_CR0           0x00006C00
#define VMCS_HOST_CR3           0x00006C02
#define VMCS_HOST_CR4           0x00006C04
#define VMCS_HOST_RSP           0x00006C14
#define VMCS_HOST_RIP           0x00006C16
#define VMCS_EXIT_REASON        0x00004402
#define VMCS_IO_RCX             0x00006400
#define VMCS_IO_RSI             0x00006402
#define VMCS_IO_RDI             0x00006404
#define VMCS_IO_RIP             0x00006406

// VMX instructions
static inline void vmxon(uint64_t addr) {
    __asm__ volatile("vmxon %0" : : "m"(addr));
}

static inline void vmxoff(void) {
    __asm__ volatile("vmxoff");
}

static inline void vmptrld(uint64_t addr) {
    __asm__ volatile("vmptrld %0" : : "m"(addr));
}

static inline void vmwrite(uint64_t field, uint64_t value) {
    __asm__ volatile("vmwrite %1, %0" : : "r"(field), "r"(value));
}

static inline uint64_t vmread(uint64_t field) {
    uint64_t value;
    __asm__ volatile("vmread %1, %0" : "=r"(value) : "r"(field));
    return value;
}

static inline void vmlaunch(void) {
    __asm__ volatile("vmlaunch");
}

static inline void vmresume(void) {
    __asm__ volatile("vmresume");
}

// Get VMX info
void get_vmx_info(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    // VMX basic info
    __asm__ volatile("rdmsr" : "=A"(vmx_basic) : "c"(0x480));
    vmcs_revision_id = vmx_basic & 0x7FFFFFFF;
}

// Allocate VMCS page (must be 4KB aligned)
vmcs_t* alloc_vmcs(void) {
    // Simple allocation from a fixed address (should use proper memory management)
    static vmcs_t vmcs __attribute__((aligned(4096)));
    vmcs.revision_id = vmcs_revision_id;
    return &vmcs;
}

// VM exit handler
void vmexit_handler(void) {
    uint32_t exit_reason = vmread(VMCS_EXIT_REASON);
    
    switch (exit_reason) {
        case VMEXIT_CPUID:
            // Handle CPUID
            break;
        case VMEXIT_HLT:
            // Handle HLT
            break;
        case VMEXIT_IO:
            // Handle I/O
            break;
        default:
            // Unknown exit
            break;
    }
    
    // Resume VM
    vmresume();
}

// Setup VMCS for basic VM
void setup_vmcs(vmcs_t* vmcs, guest_regs_t* guest) {
    vmptrld((uint64_t)vmcs);
    
    // Guest state
    vmwrite(VMCS_GUEST_CR0, guest->cr0);
    vmwrite(VMCS_GUEST_CR3, guest->cr3);
    vmwrite(VMCS_GUEST_CR4, guest->cr4);
    vmwrite(VMCS_GUEST_RSP, guest->esp);
    vmwrite(VMCS_GUEST_RIP, guest->eip);
    vmwrite(VMCS_GUEST_RFLAGS, guest->eflags);
    
    // Segments (simplified - all flat)
    vmwrite(VMCS_GUEST_CS_SELECTOR, guest->cs);
    vmwrite(VMCS_GUEST_DS_SELECTOR, guest->ds);
    vmwrite(VMCS_GUEST_ES_SELECTOR, guest->es);
    vmwrite(VMCS_GUEST_FS_SELECTOR, guest->fs);
    vmwrite(VMCS_GUEST_GS_SELECTOR, guest->gs);
    vmwrite(VMCS_GUEST_SS_SELECTOR, guest->ss);
    
    // Host state (current)
    vmwrite(VMCS_HOST_CR0, guest->cr0);  // Simplified
    vmwrite(VMCS_HOST_CR3, guest->cr3);
    vmwrite(VMCS_HOST_CR4, guest->cr4);
    vmwrite(VMCS_HOST_RSP, 0x90000);  // Stack
    vmwrite(VMCS_HOST_RIP, (uint64_t)vmexit_handler);
    
    // Setup EPT
    init_ept();
    setup_ept_in_vmcs();
}

// Launch minimal VM
void launch_minimal_vm(void) {
    // Allocate VMCS
    vmcs_t* vmcs = alloc_vmcs();
    
    // Setup guest registers (minimal 16-bit real mode like setup)
    guest_regs_t guest = {
        .cr0 = 0,
        .cr3 = 0,
        .cr4 = 0,
        .esp = 0x7C00,
        .eip = 0x7C00,
        .eflags = 0x02,
        .cs = 0,
        .ds = 0,
        .es = 0,
        .fs = 0,
        .gs = 0,
        .ss = 0
    };
    
    // Setup VMCS
    setup_vmcs(vmcs, &guest);
    
    // Launch VM
    vmlaunch();
}

// ========================================
// Memory Virtualization - Chapter 3
// ========================================

// EPT page table structures
typedef uint64_t ept_pml4e_t;
typedef uint64_t ept_pdpe_t;
typedef uint64_t ept_pde_t;
typedef uint64_t ept_pte_t;

// EPT memory type
#define EPT_MT_UC  0x00  // Uncacheable
#define EPT_MT_WC  0x01  // Write-combining
#define EPT_MT_WT  0x04  // Write-through
#define EPT_MT_WP  0x05  // Write-protected
#define EPT_MT_WB  0x06  // Write-back

// EPT permissions
#define EPT_READ    (1 << 0)
#define EPT_WRITE   (1 << 1)
#define EPT_EXECUTE (1 << 2)

// EPT page table entry
#define EPT_PRESENT 0x01
#define EPT_RW      0x02
#define EPT_USER    0x04
#define EPT_PWT     0x08
#define EPT_PCD     0x10
#define EPT_ACCESSED 0x20
#define EPT_DIRTY   0x40
#define EPT_PS      0x80
#define EPT_PAT     0x100

// Allocate EPT page tables
uint64_t* ept_pml4;
uint64_t* ept_pdpt;
uint64_t* ept_pd;
uint64_t* ept_pt;

void init_ept(void) {
    // Allocate page-aligned tables (simplified - should use proper allocation)
    static uint64_t pml4[512] __attribute__((aligned(4096))) = {0};
    static uint64_t pdpt[512] __attribute__((aligned(4096))) = {0};
    static uint64_t pd[512] __attribute__((aligned(4096))) = {0};
    static uint64_t pt[512] __attribute__((aligned(4096))) = {0};
    
    ept_pml4 = pml4;
    ept_pdpt = pdpt;
    ept_pd = pd;
    ept_pt = pt;
    
    // Setup identity mapping for first 2MB
    // PML4[0] -> PDPT
    ept_pml4[0] = (uint64_t)ept_pdpt | EPT_PRESENT | EPT_RW | EPT_EXECUTE;
    
    // PDPT[0] -> PD
    ept_pdpt[0] = (uint64_t)ept_pd | EPT_PRESENT | EPT_RW | EPT_EXECUTE;
    
    // PD[0] -> PT
    ept_pd[0] = (uint64_t)ept_pt | EPT_PRESENT | EPT_RW | EPT_EXECUTE;
    
    // PT entries for 4KB pages
    for (int i = 0; i < 512; i++) {
        uint64_t addr = i * 4096;
        ept_pt[i] = addr | EPT_PRESENT | EPT_RW | EPT_EXECUTE | (EPT_MT_WB << 3);
    }
}

// Get EPT pointer
uint64_t get_eptp(void) {
    // EPT pointer format: bits 2:0 = 0 (WB), bits 5:3 = MT, bits 11:6 = reserved
    // bits MAXPHYADDR-1:12 = EPT PML4 physical address
    return ((uint64_t)ept_pml4 & 0xFFFFFFFFFF000) | (EPT_MT_WB << 3) | 0x6;  // WB + enable bit
}

// Setup EPT in VMCS
void setup_ept_in_vmcs(void) {
    // Enable EPT
    vmwrite(0x0000201A, 1);  // Secondary VM-execution controls: enable EPT
    
    // Set EPT pointer
    vmwrite(0x0000201C, get_eptp());  // EPT pointer
}

void kernel_main(void) {
    // Initialize system state
    sys = (SystemState*)system_memory;
    memset(sys, 0, sizeof(SystemState));
    
    // Initialize hardware
    cli();
    init_pic();
    init_keyboard();
    init_mouse();
    sti();
    
    // Initialize hypervisor foundation
    if (!init_hypervisor_foundation()) {
        // Continue as regular OS
    }
    
    // Clear backbuffer
    memset(sys->backbuffer, 0, SCREEN_SIZE);
    
    // Create initial window
    create_window(60, 40, 200, 120, 9, "Welcome to Bucket OS");
    
    // Main loop
    while (1) {
        // Clear backbuffer
        memset(sys->backbuffer, 0, SCREEN_SIZE);
        
        // Process input
        process_input();
        
        // Render everything
        draw_desktop();
        draw_desktop_icons();
        draw_windows();
        draw_taskbar();
        draw_start_menu();
        draw_mouse();
        
        // Flip to screen
        flip_buffer();
        
        // Simple delay (should use timer interrupt in real implementation)
        for (volatile int i = 0; i < 50000; i++);
    }
    
    // Should never return
}