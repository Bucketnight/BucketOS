// keyboard.c: PS/2 keyboard IRQ handler (scan code -> ASCII; feeds console/shell).

/*
 * Reading guide:
 * - Purpose: keyboard.c: PS/2 keyboard IRQ handler (scan code -> ASCII; feeds console/shell).
 * - Start reading at: keyboard_initialize
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/console.h"
#include "bucketos/keyboard.h"
#include "bucketos/ports.h"
#include "bucketos/shell.h"

static const char g_scancode_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_initialize(void) {
}

static bool g_ctrl_pressed;

void keyboard_handle_irq(void) {
    const uint8_t scancode = inb(0x60);

    if (scancode == 0x1D) {
        g_ctrl_pressed = true;
        return;
    }

    if (scancode == 0x9D) {
        g_ctrl_pressed = false;
        return;
    }

    if ((scancode & 0x80u) != 0) {
        return;
    }

    const char c = g_scancode_map[scancode];
    if (console_input_is_active()) {
        if (c != 0) {
            console_push_input(c);
        }
        return;
    }

    if (g_ctrl_pressed && c == 'c') {
        shell_request_stop();
        return;
    }

    if (c != 0) {
        shell_handle_char(c);
    }
}
