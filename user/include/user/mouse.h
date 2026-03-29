// mouse.h: Userspace mouse structs/constants (read events from /dev/mouse0).

/*
 * Reading guide:
 * - Purpose: mouse.h: Userspace mouse structs/constants (read events from /dev/mouse0).
 * - Start reading at: (top of file)
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#ifndef USER_MOUSE_H
#define USER_MOUSE_H

typedef struct {
    signed char dx;
    signed char dy;
    unsigned char buttons;
    unsigned char changed;
} user_mouse_event_t;

enum {
    USER_MOUSE_BUTTON_LEFT = 0x01,
    USER_MOUSE_BUTTON_RIGHT = 0x02,
    USER_MOUSE_BUTTON_MIDDLE = 0x04
};

#endif
