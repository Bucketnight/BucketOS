#include "bucketos/terminal.h"
#include "bucketos/ports.h"

enum {
    VGA_WIDTH = 80,
    VGA_HEIGHT = 25,
    VGA_MEMORY = 0xB8000
};

static size_t g_row;
static size_t g_column;
static uint8_t g_color;
static uint16_t *const g_buffer = (uint16_t *)VGA_MEMORY;

static uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

static void terminal_update_cursor(void) {
    uint16_t position = (uint16_t)(g_row *   VGA_WIDTH + g_column);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

static void scroll_if_needed(void) {
    if (g_row < VGA_HEIGHT) {
        return;
    }

    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            g_buffer[(y - 1) * VGA_WIDTH + x] = g_buffer[y * VGA_WIDTH + x];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        g_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', g_color);
    }

    g_row = VGA_HEIGHT - 1;
}

void terminal_initialize(void) {
    g_row = 0;
    g_column = 0;
    g_color = 0x0F;
    terminal_clear();
}

void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            g_buffer[y * VGA_WIDTH + x] = vga_entry(' ', g_color);
        }
    }

    g_row = 0;
    g_column = 0;
    terminal_update_cursor();
}

void terminal_put_char(char c) {
    if (c == '\n') {
        g_column = 0;
        ++g_row;
        scroll_if_needed();
        return;
    }

    if (c == '\r') {
        g_column = 0;
        return;
    }

    if (c == '\b') {
        if (g_column > 0) {
            --g_column;
        } else if (g_row > 0) {
            --g_row;
            g_column = VGA_WIDTH - 1;
        } else {
            return;
        }

        g_buffer[g_row * VGA_WIDTH + g_column] = vga_entry(' ', g_color);
        return;
    }

    g_buffer[g_row * VGA_WIDTH + g_column] = vga_entry((unsigned char)c, g_color);

    ++g_column;
    if (g_column == VGA_WIDTH) {
        g_column = 0;
        ++g_row;
        scroll_if_needed();
    }
    terminal_update_cursor();
}

void terminal_write(const char *data) {
    while (*data != '\0') {
        terminal_put_char(*data++);
    }
}

void terminal_write_line(const char *data) {
    terminal_write(data);
    terminal_put_char('\n');
}

void terminal_set_color(uint8_t color) {
    g_color = color;
}

uint8_t terminal_get_color(void) {
    return g_color;
}
