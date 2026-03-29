// fbtest.c: Userspace framebuffer demo program.

/*
 * Reading guide:
 * - Purpose: fbtest.c: Userspace framebuffer demo program.
 * - Start reading at: main
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#include "user/fb.h"
#include "user/lib.h"

static void write_hex(int value) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[11];

    buffer[0] = '0';
    buffer[1] = 'x';
    for (int index = 0; index < 8; ++index) {
        const int shift = (7 - index) * 4;
        buffer[2 + index] = digits[(value >> shift) & 0xF];
    }
    buffer[10] = '\0';

    write_str(buffer);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const int fb = sys_open("/dev/fb0");
    if (fb < 0) {
        write_str("fbtest: open /dev/fb0 failed\n");
        return 1;
    }

    user_fb_info_t info;
    if (sys_fb_info(fb, &info) < 0) {
        write_str("fbtest: fb info failed\n");
        sys_close(fb);
        return 1;
    }

    user_fb_rect_t rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = info.width;
    rect.height = info.height;
    rect.color = 0x00121824;
    sys_fb_fill_rect(fb, &rect);

    rect.x = 40;
    rect.y = 40;
    rect.width = 260;
    rect.height = 160;
    rect.color = 0x00FF3344;
    sys_fb_fill_rect(fb, &rect);

    rect.x = 80;
    rect.y = 80;
    rect.width = 260;
    rect.height = 160;
    rect.color = 0x0033FF44;
    sys_fb_fill_rect(fb, &rect);

    rect.x = 120;
    rect.y = 120;
    rect.width = 260;
    rect.height = 160;
    rect.color = 0x003344FF;
    sys_fb_fill_rect(fb, &rect);

    {
        enum { TEST_W = 32, TEST_H = 32 };
        unsigned int pixels[TEST_W * TEST_H];

        for (int y = 0; y < TEST_H; ++y) {
            for (int x = 0; x < TEST_W; ++x) {
                const int r = x * 8;
                const int g = y * 8;
                const int b = 0x40;
                pixels[y * TEST_W + x] = (unsigned int)((r << 16) | (g << 8) | b);
            }
        }

        user_fb_blit_t blit;
        blit.dst_x = 420;
        blit.dst_y = 80;
        blit.width = TEST_W;
        blit.height = TEST_H;
        blit.source = pixels;
        blit.source_stride = TEST_W * 4;
        blit.format = USER_FB_FORMAT_XRGB8888;
        sys_fb_blit(fb, &blit);
    }

    write_str("fbtest: ");
    write_hex(info.width);
    write_str("x");
    write_hex(info.height);
    write_str(" bpp=");
    write_hex(info.bpp);
    write_str(" (close window or type anything to exit)\n");

    for (;;) {
        char c = '\0';
        const int read_count = sys_read(0, &c, 1);
        if (read_count < 0) {
            break;
        }
        if (read_count > 0) {
            break;
        }
        sys_yield();
    }

    sys_close(fb);
    return 0;
}
