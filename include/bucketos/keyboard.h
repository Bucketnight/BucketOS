// keyboard.h: PS/2 keyboard driver API.

/*
 * Reading guide:
 * - Purpose: keyboard.h: PS/2 keyboard driver API.
 * - Start reading at: keyboard_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_KEYBOARD_H
#define BUCKETOS_KEYBOARD_H

void keyboard_initialize(void);
void keyboard_handle_irq(void);

#endif
