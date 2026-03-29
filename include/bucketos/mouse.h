// mouse.h: Kernel PS/2 mouse driver API and event structure (backs /dev/mouse0).

/*
 * Reading guide:
 * - Purpose: mouse.h: Kernel PS/2 mouse driver API and event structure (backs /dev/mouse0).
 * - Start reading at: mouse_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_MOUSE_H
#define BUCKETOS_MOUSE_H

#include "bucketos/common.h"

typedef struct {
    int8_t dx;
    int8_t dy;
    uint8_t buttons;
    uint8_t changed;
} mouse_event_t;

void mouse_initialize(void);
void mouse_handle_irq(void);
size_t mouse_read_events(mouse_event_t *events, size_t max_events);

#endif
