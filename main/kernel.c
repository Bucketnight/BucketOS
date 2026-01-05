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

// ========================================
// Global State
// ========================================

static SystemState* sys;
static uint8_t system_memory[sizeof(SystemState)] __attribute__((aligned(4)));

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

static inline void sti(void) {
    __asm__ volatile ("sti");
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

// Font Data (8x8)
// ========================================

static const uint8_t font_data[256 * 8] = {
    // Space (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00,
    // " (34)
    0x36, 0x36, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    // # (35)
    0x24, 0x24, 0x7F, 0x24, 0x7F, 0x24, 0x24, 0x00,
    // $ (36)
    0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x18, 0x00,
    // % (37)
    0x62, 0x66, 0x0C, 0x18, 0x30, 0x66, 0x46, 0x00,
    // & (38)
    0x1C, 0x36, 0x36, 0x1C, 0x35, 0x66, 0x3A, 0x00,
    // ' (39)
    0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ( (40)
    0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00,
    // ) (41)
    0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00,
    // * (42)
    0x00, 0x24, 0x18, 0x7E, 0x18, 0x24, 0x00, 0x00,
    // + (43)
    0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00,
    // , (44)
    0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00,
    // - (45)
    0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    // . (46)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
    // / (47)
    0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00,
    // 0-9 (48-57)
    0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00,  // 0
    0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00,  // 1
    0x3C, 0x66, 0x06, 0x1C, 0x30, 0x66, 0x7E, 0x00,  // 2
    0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00,  // 3
    0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00,  // 4
    0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00,  // 5
    0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00,  // 6
    0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,  // 7
    0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00,  // 8
    0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00,  // 9
    // : (58)
    0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00,
    // ; (59)
    0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30, 0x00,
    // < (60)
    0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00,
    // = (61)
    0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00,
    // > (62)
    0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00,
    // ? (63)
    0x3C, 0x66, 0x06, 0x0C, 0x18, 0x00, 0x18, 0x00,
    // @ (64)
    0x3C, 0x66, 0x6E, 0x6E, 0x60, 0x62, 0x3C, 0x00,
    // A-Z (65-90)
    0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00,  // A
    0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00,  // B
    0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00,  // C
    0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00,  // D
    0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00,  // E
    0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00,  // F
    0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00,  // G
    0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,  // H
    0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,  // I
    0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00,  // J
    0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00,  // K
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00,  // L
    0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00,  // M
    0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00,  // N
    0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00,  // O
    0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00,  // P
    0x3C, 0x66, 0x66, 0x66, 0x6E, 0x3C, 0x0E, 0x00,  // Q
    0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00,  // R
    0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00,  // S
    0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,  // T
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00,  // U
    0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00,  // V
    0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00,  // W
    0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00,  // X
    0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00,  // Y
    0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00,  // Z
    // [ (91)
    0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00,
    // \ (92)
    0x80, 0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00,
    // ] (93)
    0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00,
    // ^ (94)
    0x18, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00,
    // _ (95)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    // ` (96)
    0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    // a-z (97-122)
    0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00,  // a
    0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00,  // b
    0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00,  // c
    0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00,  // d
    0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00,  // e
    0x1C, 0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x00,  // f
    0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C,  // g
    0x60, 0x60, 0x6C, 0x76, 0x66, 0x66, 0x66, 0x00,  // h
    0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00,  // i
    0x0C, 0x00, 0x1C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38,  // j
    0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00,  // k
    0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,  // l
    0x00, 0x00, 0x6C, 0x7E, 0x7E, 0x6A, 0x62, 0x00,  // m
    0x00, 0x00, 0x6C, 0x76, 0x66, 0x66, 0x66, 0x00,  // n
    0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00,  // o
    0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60,  // p
    0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06,  // q
    0x00, 0x00, 0x6C, 0x76, 0x60, 0x60, 0x60, 0x00,  // r
    0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00,  // s
    0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x1C, 0x00,  // t
    0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00,  // u
    0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00,  // v
    0x00, 0x00, 0x63, 0x63, 0x6B, 0x7F, 0x36, 0x00,  // w
    0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00,  // x
    0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C,  // y
    0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00,  // z
    // { (123)
    0x0E, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0E, 0x00,
    // | (124)
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    // } (125)
    0x70, 0x18, 0x18, 0x0C, 0x18, 0x18, 0x70, 0x00,
    // ~ (126)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // DEL (127)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
    // Process up to 10 mouse packets per frame to avoid infinite loops
    int packets_processed = 0;
    const int max_packets = 10;
    
    while ((inb(MOUSE_STATUS_PORT) & 0x21) == 0x21 && packets_processed < max_packets) {  // Bit 0 and bit 5 must be set
        uint8_t data = inb(MOUSE_DATA_PORT);
        sys->mouse.packet_buffer[sys->mouse.packet_index] = data;
        sys->mouse.packet_index++;
        
        if (sys->mouse.packet_index < 3) {
            continue;  // Wait for complete packet
        }
        
        sys->mouse.packet_index = 0;
        packets_processed++;
        
        // Check validity - bit 3 should be set for valid packets
        if (!(sys->mouse.packet_buffer[0] & 0x08)) {
            continue;  // Invalid packet, skip
        }
        
        // Store previous button state
        sys->mouse.buttons_prev = sys->mouse.buttons;
        
        // Extract button state (bits 0-2)
        sys->mouse.buttons = sys->mouse.packet_buffer[0] & 0x07;
        
        // Extract movement (9-bit two's complement, but we use 8-bit signed)
        int32_t dx = (int32_t)(int8_t)sys->mouse.packet_buffer[1];
        int32_t dy = -(int32_t)(int8_t)sys->mouse.packet_buffer[2];  // Invert Y axis (screen Y increases downward)
        
        // Apply movement with sensitivity adjustment
        sys->mouse.x += dx;
        sys->mouse.y += dy;
        
        // Clamp to screen bounds
        if (sys->mouse.x < 0) sys->mouse.x = 0;
        if (sys->mouse.x >= SCREEN_WIDTH - 8) sys->mouse.x = SCREEN_WIDTH - 8;
        if (sys->mouse.y < 0) sys->mouse.y = 0;
        if (sys->mouse.y >= SCREEN_HEIGHT - 8) sys->mouse.y = SCREEN_HEIGHT - 8;
    }
}

// ========================================
// Keyboard Driver
// ========================================

void init_keyboard(void) {
    sys->keyboard.head = 0;
    sys->keyboard.tail = 0;
}

void read_keyboard(void) {
    uint8_t status = inb(KB_STATUS_PORT);
    
    // Check if data is available and it's not mouse data (bit 5 = 0 for keyboard)
    if (!(status & 0x01) || (status & 0x20)) {
        return;  // No keyboard data or it's mouse data
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
        draw_string(win->x + 10, win->y + 65, "I/O Trap: Enabled", 10);
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
        // Arrow keys disabled - use mouse only
        /*
        else if (scancode == 0x48) sys->mouse.y -= 4;  // Up
        else if (scancode == 0x50) sys->mouse.y += 4;  // Down
        else if (scancode == 0x4B) sys->mouse.x -= 4;  // Left
        else if (scancode == 0x4D) sys->mouse.x += 4;  // Right
        */
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
void handle_cpuid_exit(void);
void handle_io_exit(void);
void handle_hlt_exit(void);
void handle_exception_exit(void);
void handle_msr_exit(void);
void handle_port_out(uint16_t port, uint32_t value, uint8_t size);
uint32_t handle_port_in(uint16_t port, uint8_t size);
uint64_t read_msr(uint32_t msr);
void write_msr(uint32_t msr, uint64_t value);
void setup_vmcs_features(void);

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

// Additional VMCS fields for I/O and MSR handling
#define VMCS_EXIT_QUALIFICATION 0x00006400
#define VMCS_GUEST_RAX          0x0000681E
#define VMCS_GUEST_RBX          0x0000681C
#define VMCS_GUEST_RCX          0x0000681A
#define VMCS_GUEST_RDX          0x00006818

// VM Exit Reasons
#define VMEXIT_CPUID            10
#define VMEXIT_HLT              12
#define VMEXIT_IO               30
#define VMEXIT_EXCEPTION        0
#define VMEXIT_MSR_READ        31
#define VMEXIT_MSR_WRITE       32
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
            // Handle CPUID - emulate basic features
            handle_cpuid_exit();
            break;
        case VMEXIT_HLT:
            // Handle HLT - inject interrupt or continue
            handle_hlt_exit();
            break;
        case VMEXIT_IO:
            // Handle I/O instruction trapping
            handle_io_exit();
            break;
        case VMEXIT_EXCEPTION:
            // Handle exceptions
            handle_exception_exit();
            break;
        case VMEXIT_MSR_READ:
        case VMEXIT_MSR_WRITE:
            // Handle MSR access
            handle_msr_exit();
            break;
        default:
            // Unknown exit - log and halt
            break;
    }
    
    // Resume VM
    vmresume();
}

// Handle CPUID exit
void handle_cpuid_exit(void) {
    uint32_t eax, ebx, ecx, edx;
    uint32_t leaf = vmread(VMCS_GUEST_RAX);
    
    // Emulate basic CPUID
    switch (leaf) {
        case 0:  // Vendor ID
            eax = 1;  // Max leaf
            ebx = 0x756E6547;  // "Genu"
            ecx = 0x6C65746E;  // "ntel"
            edx = 0x49656E69;  // "ineI"
            break;
        case 1:  // Feature flags
            eax = 0x0001067A;  // Family/Model/Stepping
            ebx = 0;           // Brand index
            ecx = 0;           // Extended features
            edx = 0x00000001;  // FPU present
            break;
        default:
            eax = ebx = ecx = edx = 0;
            break;
    }
    
    // Write results back to guest registers
    vmwrite(VMCS_GUEST_RAX, eax);
    vmwrite(VMCS_GUEST_RBX, ebx);
    vmwrite(VMCS_GUEST_RCX, ecx);
    vmwrite(VMCS_GUEST_RDX, edx);
}

// Handle I/O exit
void handle_io_exit(void) {
    uint32_t qualification = vmread(VMCS_EXIT_QUALIFICATION);
    
    // Extract I/O details from qualification
    uint16_t port = qualification >> 16;
    uint8_t size = (qualification >> 5) & 3;  // 0=1byte, 1=2bytes, 3=4bytes
    uint8_t direction = qualification & 1;    // 0=out, 1=in
    uint8_t string = (qualification >> 4) & 1; // String I/O
    
    if (direction == 0) {  // OUT instruction
        uint32_t value = vmread(VMCS_GUEST_RAX);
        // Handle output to port
        handle_port_out(port, value, size);
    } else {  // IN instruction
        uint32_t value = handle_port_in(port, size);
        vmwrite(VMCS_GUEST_RAX, value);
    }
}

// Handle HLT exit
void handle_hlt_exit(void) {
    // For now, just advance RIP past the HLT
    uint32_t rip = vmread(VMCS_GUEST_RIP);
    vmwrite(VMCS_GUEST_RIP, rip + 1);
}

// Handle exception exit
void handle_exception_exit(void) {
    uint32_t qualification = vmread(VMCS_EXIT_QUALIFICATION);
    uint8_t vector = qualification & 0xFF;
    
    // Handle exception based on vector
    switch (vector) {
        case 0:  // Divide by zero
        case 6:  // Invalid opcode
        case 13: // General protection fault
            // Inject exception back to guest or handle
            break;
    }
}

// Handle MSR exit
void handle_msr_exit(void) {
    uint32_t exit_reason = vmread(VMCS_EXIT_REASON);
    uint32_t msr = vmread(VMCS_GUEST_RCX);
    
    if (exit_reason == VMEXIT_MSR_READ) {
        uint64_t value = read_msr(msr);
        vmwrite(VMCS_GUEST_RAX, value & 0xFFFFFFFF);
        vmwrite(VMCS_GUEST_RDX, value >> 32);
    } else {  // MSR write
        uint32_t low = vmread(VMCS_GUEST_RAX);
        uint32_t high = vmread(VMCS_GUEST_RDX);
        write_msr(msr, ((uint64_t)high << 32) | low);
    }
}

// MSR access functions
uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void write_msr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile ("wrmsr" : : "a"(low), "d"(high), "c"(msr));
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
    setup_vmcs_features();
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

// Setup VMCS features (EPT + I/O trapping)
void setup_vmcs_features(void) {
    // Enable EPT
    vmwrite(0x0000201A, 1);  // Secondary VM-execution controls: enable EPT
    
    // Set EPT pointer
    vmwrite(0x0000201C, get_eptp());  // EPT pointer
    
    // Setup I/O bitmap for trapping
    static uint8_t io_bitmap_a[4096] __attribute__((aligned(4096)));
    static uint8_t io_bitmap_b[4096] __attribute__((aligned(4096)));
    
    // Initialize I/O bitmaps (trap all I/O initially)
    memset(io_bitmap_a, 0xFF, 4096);
    memset(io_bitmap_b, 0xFF, 4096);
    
    // Allow some ports for guest (e.g., VGA, keyboard)
    // Clear bits for ports we want to pass through
    io_bitmap_a[0x3C0 >> 3] &= ~(1 << (0x3C0 & 7));  // VGA index
    io_bitmap_a[0x3C1 >> 3] &= ~(1 << (0x3C1 & 7));  // VGA data read
    io_bitmap_a[0x3C4 >> 3] &= ~(1 << (0x3C4 & 7));  // VGA sequencer index
    io_bitmap_a[0x3C5 >> 3] &= ~(1 << (0x3C5 & 7));  // VGA sequencer data
    io_bitmap_a[0x3CE >> 3] &= ~(1 << (0x3CE & 7));  // VGA graphics controller index
    io_bitmap_a[0x3CF >> 3] &= ~(1 << (0x3CF & 7));  // VGA graphics controller data
    
    // Set I/O bitmap addresses
    vmwrite(0x00002000, (uint32_t)io_bitmap_a);  // I/O bitmap A
    vmwrite(0x00002002, (uint32_t)io_bitmap_b);  // I/O bitmap B
    
    // Enable I/O bitmap in primary VM-execution controls
    uint32_t exec_controls = vmread(0x00004004);
    exec_controls |= (1 << 25);  // Enable I/O bitmap
    vmwrite(0x00004004, exec_controls);
}

// Port I/O handlers (simplified)
void handle_port_out(uint16_t port, uint32_t value, uint8_t size) {
    switch (port) {
        case 0x3F8:  // COM1 data
            // Serial output - could implement later
            break;
        case 0x3F9:  // COM1 interrupt enable
            break;
        case 0x3C8:  // VGA DAC address
        case 0x3C9:  // VGA DAC data
            // VGA palette - allow guest access
            outb(port, (uint8_t)value);
            break;
        // Add more ports as needed
    }
}

uint32_t handle_port_in(uint16_t port, uint8_t size) {
    switch (port) {
        case 0x3F8:  // COM1 data
            return 0;  // No data available
        case 0x3FD:  // COM1 line status
            return 0x60;  // Transmitter empty, ready
        case 0x3C9:  // VGA DAC data
            return inb(port);  // Allow reading VGA palette
        default:
            return 0xFFFFFFFF;  // Default value
    }
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
    // Note: Interrupts disabled - using polling for input
    
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