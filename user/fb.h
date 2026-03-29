// fb.h: Userspace framebuffer syscall wrappers and structs.

/*
 * Reading guide:
 * - Purpose: fb.h: Userspace framebuffer syscall wrappers and structs.
 * - Start reading at: sys_fb_info
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#ifndef USER_FB_H
#define USER_FB_H

#include "user/syscall.h"

typedef struct {
    int width;
    int height;
    int pitch;
    int bpp;
    int type;
} user_fb_info_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int color;
} user_fb_rect_t;

typedef struct {
    int dst_x;
    int dst_y;
    int width;
    int height;
    const void *source;
    int source_stride;
    int format;
} user_fb_blit_t;

enum {
    USER_FB_FORMAT_XRGB8888 = 1
};

static inline int sys_fb_info(int fb_fd, user_fb_info_t *info) {
    return user_syscall2(USER_SYSCALL_FB_INFO, fb_fd, (int)info);
}

static inline int sys_fb_putpixel(int fb_fd, int x, int y, int color) {
    return user_syscall4(USER_SYSCALL_FB_PUTPIXEL, fb_fd, x, y, color);
}

static inline int sys_fb_fill_rect(int fb_fd, const user_fb_rect_t *rect) {
    return user_syscall2(USER_SYSCALL_FB_FILL_RECT, fb_fd, (int)rect);
}

static inline int sys_fb_blit(int fb_fd, const user_fb_blit_t *blit) {
    return user_syscall2(USER_SYSCALL_FB_BLIT, fb_fd, (int)blit);
}

#endif
