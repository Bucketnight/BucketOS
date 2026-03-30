// wm.c: Userspace window server (draws windows and cursor; reads /dev/mouse0 + /dev/console).

/*
 * Reading guide:
 * - Purpose: wm.c: Userspace window server (draws windows and cursor; reads /dev/mouse0 + /dev/console).
 * - Start reading at: main
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#include "user/fb.h"
#include "user/lib.h"
#include "user/mouse.h"

enum {
    FONT_WIDTH = 8,
    FONT_HEIGHT = 16,
    WINDOW_TEXT_MAX = 256,
    WINDOW_MAX = 2,
    CURSOR_WIDTH = 10,
    CURSOR_HEIGHT = 16,
    TASKBAR_HEIGHT = 28,
    TASKBAR_PADDING = 4,
    START_BUTTON_WIDTH = 72,
    START_MENU_WIDTH = 160,
    START_MENU_ITEM_HEIGHT = 20,
    START_MENU_ITEMS = 3,
    CLOSE_BUTTON_SIZE = 16
};

enum {
    COLOR_DESKTOP_BG = 0x00121824,
    COLOR_TASKBAR_BG = 0x00101010,
    COLOR_TASKBAR_BORDER = 0x00333333,
    COLOR_TASKBAR_BUTTON_BG = 0x00222222,
    COLOR_TASKBAR_TEXT = 0x00E6E6E6,
    COLOR_MENU_BG = 0x00181818,
    COLOR_MENU_ITEM_BG = 0x00222222,
    COLOR_MENU_TEXT = 0x00FFFFFF
};

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char *title;
    char text[WINDOW_TEXT_MAX];
    int text_length;
    int focused;
    int visible;
    int counter;
} window_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} rect_t;

static const unsigned char g_font[96][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 },
    { 0x6C, 0x6C, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00 },
    { 0x18, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x18, 0x00 },
    { 0x00, 0xC6, 0xCC, 0x18, 0x30, 0x66, 0xC6, 0x00 },
    { 0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00 },
    { 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00 },
    { 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00 },
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 },
    { 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00 },
    { 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00 },
    { 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00 },
    { 0x7C, 0xC6, 0xCE, 0xD6, 0xE6, 0xC6, 0x7C, 0x00 },
    { 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00 },
    { 0x7C, 0xC6, 0x0E, 0x1C, 0x70, 0xC0, 0xFE, 0x00 },
    { 0x7C, 0xC6, 0x06, 0x3C, 0x06, 0xC6, 0x7C, 0x00 },
    { 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00 },
    { 0xFE, 0xC0, 0xFC, 0x06, 0x06, 0xC6, 0x7C, 0x00 },
    { 0x3C, 0x60, 0xC0, 0xFC, 0xC6, 0xC6, 0x7C, 0x00 },
    { 0xFE, 0xC6, 0x0E, 0x1C, 0x30, 0x30, 0x30, 0x00 },
    { 0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00 },
    { 0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0x78, 0x00 },
    { 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00 },
    { 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30, 0x00 },
    { 0x0E, 0x1C, 0x38, 0x70, 0x38, 0x1C, 0x0E, 0x00 },
    { 0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00 },
    { 0x70, 0x38, 0x1C, 0x0E, 0x1C, 0x38, 0x70, 0x00 },
    { 0x7C, 0xC6, 0x0E, 0x1C, 0x18, 0x00, 0x18, 0x00 },
    { 0x7C, 0xC6, 0xDE, 0xDE, 0xDE, 0xC0, 0x78, 0x00 },
    { 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0x00 },
    { 0xFC, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC, 0x00 },
    { 0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00 },
    { 0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00 },
    { 0xFE, 0x62, 0x68, 0x78, 0x68, 0x62, 0xFE, 0x00 },
    { 0xFE, 0x62, 0x68, 0x78, 0x68, 0x60, 0xF0, 0x00 },
    { 0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3E, 0x00 },
    { 0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00 },
    { 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00 },
    { 0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00 },
    { 0xE6, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0xE6, 0x00 },
    { 0xF0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00 },
    { 0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0x00 },
    { 0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00 },
    { 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00 },
    { 0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00 },
    { 0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0xCC, 0x7A, 0x00 },
    { 0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00 },
    { 0x7C, 0xC6, 0xE0, 0x7C, 0x0E, 0xC6, 0x7C, 0x00 },
    { 0x7E, 0x7E, 0x5A, 0x18, 0x18, 0x18, 0x3C, 0x00 },
    { 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00 },
    { 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00 },
    { 0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00 },
    { 0xC6, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0xC6, 0x00 },
    { 0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x00 },
    { 0xFE, 0xC6, 0x8C, 0x18, 0x32, 0x66, 0xFE, 0x00 },
    { 0x78, 0x60, 0x60, 0x60, 0x60, 0x60, 0x78, 0x00 },
    { 0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00 },
    { 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00 },
    { 0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF },
    { 0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0x06, 0x7E, 0xC6, 0x7E, 0x00 },
    { 0xE0, 0x60, 0x7C, 0x66, 0x66, 0x66, 0xDC, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC0, 0xC6, 0x7C, 0x00 },
    { 0x1C, 0x0C, 0x7C, 0xCC, 0xCC, 0xCC, 0x76, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xFE, 0xC0, 0x7C, 0x00 },
    { 0x38, 0x6C, 0x60, 0xF8, 0x60, 0x60, 0xF0, 0x00 },
    { 0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8 },
    { 0xE0, 0x60, 0x6C, 0x76, 0x66, 0x66, 0xE6, 0x00 },
    { 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00 },
    { 0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78 },
    { 0xE0, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0xE6, 0x00 },
    { 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00 },
    { 0x00, 0x00, 0xCC, 0xFE, 0xFE, 0xD6, 0xC6, 0x00 },
    { 0x00, 0x00, 0xF8, 0xCC, 0xCC, 0xCC, 0xCC, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x00 },
    { 0x00, 0x00, 0xDC, 0x66, 0x66, 0x7C, 0x60, 0xF0 },
    { 0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x1E },
    { 0x00, 0x00, 0xDC, 0x76, 0x66, 0x60, 0xF0, 0x00 },
    { 0x00, 0x00, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x00 },
    { 0x10, 0x30, 0x7C, 0x30, 0x30, 0x36, 0x1C, 0x00 },
    { 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00 },
    { 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00 },
    { 0x00, 0x00, 0xC6, 0xD6, 0xFE, 0xFE, 0x6C, 0x00 },
    { 0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00 },
    { 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0xFC },
    { 0x00, 0x00, 0xFE, 0x4C, 0x18, 0x32, 0xFE, 0x00 },
    { 0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00 },
    { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 },
    { 0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00 },
    { 0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0x00 }
};

static int clamp_int(int value, int min, int max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static void u32_to_dec(char *out, unsigned int value) {
    char tmp[11];
    int count = 0;

    do {
        tmp[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u && count < (int)sizeof(tmp));

    int pos = 0;
    while (count > 0) {
        out[pos++] = tmp[--count];
    }
    out[pos] = '\0';
}

static void build_counter_label(char *out, unsigned int value) {
    char num[11];
    u32_to_dec(num, value);

    const char *prefix = "count=";
    int pos = 0;
    for (int i = 0; prefix[i] != '\0'; ++i) {
        out[pos++] = prefix[i];
    }
    for (int i = 0; num[i] != '\0'; ++i) {
        out[pos++] = num[i];
    } 
    out[pos] = '\0';
}

static rect_t rect_make(int x, int y, int width, int height) {
    rect_t rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    return rect;
}

static int rect_intersect(const rect_t *a, const rect_t *b, rect_t *out) {
    int ax2;
    int ay2;
    int bx2;
    int by2;

    if (a == 0 || b == 0 || out == 0) {
        return 0;
    }

    ax2 = a->x + a->width;
    ay2 = a->y + a->height;
    bx2 = b->x + b->width;
    by2 = b->y + b->height;

    const int x1 = (a->x > b->x) ? a->x : b->x;
    const int y1 = (a->y > b->y) ? a->y : b->y;
    const int x2 = (ax2 < bx2) ? ax2 : bx2;
    const int y2 = (ay2 < by2) ? ay2 : by2;

    if (x2 <= x1 || y2 <= y1) {
        return 0;
    }

    out->x = x1;
    out->y = y1;
    out->width = x2 - x1;
    out->height = y2 - y1;
    return 1;
}

static rect_t rect_union(rect_t a, rect_t b) {
    const int x1 = (a.x < b.x) ? a.x : b.x;
    const int y1 = (a.y < b.y) ? a.y : b.y;
    const int x2 = ((a.x + a.width) > (b.x + b.width)) ? (a.x + a.width) : (b.x + b.width);
    const int y2 = ((a.y + a.height) > (b.y + b.height)) ? (a.y + a.height) : (b.y + b.height);
    return rect_make(x1, y1, x2 - x1, y2 - y1);
}

static int rect_contains_point(rect_t rect, int x, int y) {
    return x >= rect.x
        && y >= rect.y
        && x < rect.x + rect.width
        && y < rect.y + rect.height;
}

static rect_t rect_clamp_to_screen(rect_t rect, const user_fb_info_t *info) {
    if (info == 0) {
        return rect_make(0, 0, 0, 0);
    }

    if (rect.x < 0) {
        rect.width += rect.x;
        rect.x = 0;
    }
    if (rect.y < 0) {
        rect.height += rect.y;
        rect.y = 0;
    }
    if (rect.x + rect.width > info->width) {
        rect.width = info->width - rect.x;
    }
    if (rect.y + rect.height > info->height) {
        rect.height = info->height - rect.y;
    }
    if (rect.width < 0) {
        rect.width = 0;
    }
    if (rect.height < 0) {
        rect.height = 0;
    }

    return rect;
}

static void fb_fill_rect(int fb, int x, int y, int width, int height, int color) {
    user_fb_rect_t rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    rect.color = color;
    sys_fb_fill_rect(fb, &rect);
}

static void fb_fill_rect_clipped(int fb, rect_t rect, rect_t clip, int color) {
    rect_t visible;
    if (!rect_intersect(&rect, &clip, &visible)) {
        return;
    }

    fb_fill_rect(fb, visible.x, visible.y, visible.width, visible.height, color);
}

static void fb_blit_glyph(
    int fb, int x, int y, char ch, int fg_color, int bg_color) {
    unsigned int pixels[FONT_WIDTH * FONT_HEIGHT];
    const unsigned char index = (unsigned char)ch;
    const unsigned char *glyph =
        (index >= 32u && index <= 127u) ? g_font[index - 32u] : g_font[0];

    for (int py = 0; py < FONT_HEIGHT; ++py) {
        const unsigned char row_bits = glyph[(unsigned int)py / 2u];
        for (int px = 0; px < FONT_WIDTH; ++px) {
            const unsigned int bit_set = (row_bits & (unsigned char)(0x80u >> px)) != 0u; 
            pixels[py * FONT_WIDTH + px] = (unsigned int)(bit_set ? fg_color : bg_color);
        }
    }

    user_fb_blit_t blit;
    blit.dst_x = x;
    blit.dst_y = y;
    blit.width = FONT_WIDTH;
    blit.height = FONT_HEIGHT;
    blit.source = pixels;
    blit.source_stride = FONT_WIDTH * 4;
    blit.format = USER_FB_FORMAT_XRGB8888;
    sys_fb_blit(fb, &blit);
}

static void fb_blit_glyph_clipped(
    int fb, int x, int y, char ch, int fg_color, int bg_color, rect_t clip) {
    rect_t glyph_rect = rect_make(x, y, FONT_WIDTH, FONT_HEIGHT);
    rect_t visible;

    if (!rect_intersect(&glyph_rect, &clip, &visible)) {
        return;
    }

    unsigned int pixels[FONT_WIDTH * FONT_HEIGHT];
    const unsigned char index = (unsigned char)ch;
    const unsigned char *glyph =
        (index >= 32u && index <= 127u) ? g_font[index - 32u] : g_font[0];

    const int offset_x = visible.x - x;
    const int offset_y = visible.y - y;

    for (int py = 0; py < visible.height; ++py) {
        const int glyph_y = py + offset_y;
        const unsigned char row_bits = glyph[(unsigned int)glyph_y / 2u];

        for (int px = 0; px < visible.width; ++px) {
            const int glyph_x = px + offset_x;
            const unsigned int bit_set = (row_bits & (unsigned char)(0x80u >> glyph_x)) != 0u;
            pixels[py * visible.width + px] = (unsigned int)(bit_set ? fg_color : bg_color);
        }
    }

    user_fb_blit_t blit;
    blit.dst_x = visible.x;
    blit.dst_y = visible.y;
    blit.width = visible.width;
    blit.height = visible.height;
    blit.source = pixels;
    blit.source_stride = visible.width * 4;
    blit.format = USER_FB_FORMAT_XRGB8888;
    sys_fb_blit(fb, &blit);
}

static void fb_draw_string_clipped(
    int fb, int x, int y, const char *text, int fg_color, int bg_color, rect_t clip) {
    int cursor_x = x;
    int cursor_y = y;

    for (int index = 0; text[index] != '\0'; ++index) {
        const char c = text[index];

        if (c == '\n') {
            cursor_x = x;
            cursor_y += FONT_HEIGHT;
            continue;
        }

        fb_blit_glyph_clipped(fb, cursor_x, cursor_y, c, fg_color, bg_color, clip);

        cursor_x += FONT_WIDTH;
    }
}

static void fb_draw_string(
    int fb, int x, int y, const char *text, int fg_color, int bg_color) {
    int cursor_x = x;
    int cursor_y = y;

    for (int index = 0; text[index] != '\0'; ++index) {
        const char c = text[index];

        if (c == '\n') {
            cursor_x = x;
            cursor_y += FONT_HEIGHT;
            continue;
        }

        fb_blit_glyph(fb, cursor_x, cursor_y, c, fg_color, bg_color);
        cursor_x += FONT_WIDTH;
    }
}

static int window_contains(const window_t *window, int x, int y) {
    return window->visible
        && x >= window->x
        && y >= window->y
        && x < window->x + window->width
        && y < window->y + window->height;
}

static rect_t window_titlebar_rect(const window_t *window) {
    return rect_make(window->x + 2, window->y + 2, window->width - 4, 22);
}

static rect_t window_close_button_rect(const window_t *window) {
    return rect_make(window->x + window->width - CLOSE_BUTTON_SIZE - 6,
        window->y + 5,
        CLOSE_BUTTON_SIZE,
        CLOSE_BUTTON_SIZE);
}

static rect_t window_widget_button_rect(const window_t *window) {
    return rect_make(window->x + 8, window->y + window->height - 28, 84, 20);
}

static int first_visible_window(const window_t windows[WINDOW_MAX]) {
    for (int i = 0; i < WINDOW_MAX; ++i) {
        if (windows[i].visible) {
            return i;
        }
    }
    return -1;
}

static void window_set_text(window_t *window, const char *text) {
    if (window == 0 || text == 0) {
        return;
    }

    int length = 0;
    while (text[length] != '\0' && length + 1 < WINDOW_TEXT_MAX) {
        window->text[length] = text[length];
        ++length;
    }

    window->text[length] = '\0';
    window->text_length = length;
}

static void window_draw_clipped(int fb, const window_t *window, rect_t clip) {
    if (!window->visible) {
        return;
    }

    const int border = window->focused ? 0x00FFD34D : 0x003A3A3A;
    const int title_bg = window->focused ? 0x002E7DFF : 0x00222222;
    const int title_fg = 0x00FFFFFF;
    const int body_bg = 0x00111111;
    const int body_fg = 0x00E6E6E6;
    const int widget_bg = 0x00202020;

    const rect_t border_rect = rect_make(window->x, window->y, window->width, window->height);
    const rect_t title_rect = window_titlebar_rect(window);
    const rect_t body_rect = rect_make(window->x + 2, window->y + 24, window->width - 4, window->height - 26);

    fb_fill_rect_clipped(fb, border_rect, clip, border);
    fb_fill_rect_clipped(fb, title_rect, clip, title_bg);
    fb_fill_rect_clipped(fb, body_rect, clip, body_bg);

    fb_draw_string_clipped(fb, window->x + 8, window->y + 6, window->title, title_fg, title_bg, clip);

    const rect_t close_rect = window_close_button_rect(window);
    const int close_bg = window->focused ? 0x00C62828 : 0x00444444;
    fb_fill_rect_clipped(fb, close_rect, clip, close_bg);
    fb_blit_glyph_clipped(fb, close_rect.x + 4, close_rect.y + 1, 'X', 0x00FFFFFF, close_bg, clip);

    if (window->text_length > 0) {
        char line[WINDOW_TEXT_MAX + 1];
        int length = window->text_length;
        if (length > WINDOW_TEXT_MAX) {
            length = WINDOW_TEXT_MAX;
        }
        for (int i = 0; i < length; ++i) {
            line[i] = window->text[i];
        }
        line[length] = '\0';

        fb_draw_string_clipped(fb, window->x + 8, window->y + 32, line, body_fg, body_bg, clip);
    }

    // Simple widget: a clickable button that increments a per-window counter.
    const rect_t widget_rect = window_widget_button_rect(window);
    fb_fill_rect_clipped(fb, widget_rect, clip, widget_bg);
    fb_draw_string_clipped(fb, widget_rect.x + 8, widget_rect.y + 2, "Click", 0x00FFFFFF, widget_bg, clip);

    char counter_text[32];
    build_counter_label(counter_text, (unsigned int)window->counter);
    fb_draw_string_clipped(fb, widget_rect.x + widget_rect.width + 12, widget_rect.y + 2,
        counter_text, body_fg, body_bg, clip);
}

static void window_draw(int fb, const window_t *window) {
    if (!window->visible) {
        return;
    }

    const int border = window->focused ? 0x00FFD34D : 0x003A3A3A;
    const int title_bg = window->focused ? 0x002E7DFF : 0x00222222;
    const int title_fg = 0x00FFFFFF;
    const int body_bg = 0x00111111;
    const int body_fg = 0x00E6E6E6;
    const int widget_bg = 0x00202020;

    fb_fill_rect(fb, window->x, window->y, window->width, window->height, border);
    fb_fill_rect(fb, window->x + 2, window->y + 2, window->width - 4, 22, title_bg);
    fb_fill_rect(fb, window->x + 2, window->y + 24, window->width - 4, window->height - 26, body_bg);
    fb_draw_string(fb, window->x + 8, window->y + 6, window->title, title_fg, title_bg);

    const rect_t close_rect = window_close_button_rect(window);
    const int close_bg = window->focused ? 0x00C62828 : 0x00444444;
    fb_fill_rect(fb, close_rect.x, close_rect.y, close_rect.width, close_rect.height, close_bg);
    fb_blit_glyph(fb, close_rect.x + 4, close_rect.y + 1, 'X', 0x00FFFFFF, close_bg);

    if (window->text_length > 0) {
        char line[WINDOW_TEXT_MAX + 1];
        int length = window->text_length;
        if (length > WINDOW_TEXT_MAX) {
            length = WINDOW_TEXT_MAX;
        }
        for (int i = 0; i < length; ++i) {
            line[i] = window->text[i];
        }
        line[length] = '\0';

        fb_draw_string(fb, window->x + 8, window->y + 32, line, body_fg, body_bg);
    }

    const rect_t widget_rect = window_widget_button_rect(window);
    fb_fill_rect(fb, widget_rect.x, widget_rect.y, widget_rect.width, widget_rect.height, widget_bg);
    fb_draw_string(fb, widget_rect.x + 8, widget_rect.y + 2, "Click", 0x00FFFFFF, widget_bg);

    char counter_text[32];
    build_counter_label(counter_text, (unsigned int)window->counter);
    fb_draw_string(fb, widget_rect.x + widget_rect.width + 12, widget_rect.y + 2,
        counter_text, body_fg, body_bg);
}

static int taskbar_top(const user_fb_info_t *info) {
    return info->height - TASKBAR_HEIGHT;
}

static rect_t taskbar_rect(const user_fb_info_t *info) {
    return rect_make(0, taskbar_top(info), info->width, TASKBAR_HEIGHT);
}

static rect_t taskbar_start_button_rect(const user_fb_info_t *info) {
    const int y = taskbar_top(info) + TASKBAR_PADDING;
    return rect_make(8, y, START_BUTTON_WIDTH, TASKBAR_HEIGHT - 2 * TASKBAR_PADDING);
}

static rect_t start_menu_rect(const user_fb_info_t *info) {
    const int menu_height = START_MENU_ITEMS * START_MENU_ITEM_HEIGHT + 8;
    const int x = 8;
    const int y = taskbar_top(info) - menu_height - 2;
    return rect_make(x, y, START_MENU_WIDTH, menu_height);
}

static int start_menu_item_at(const user_fb_info_t *info, int x, int y) {
    const rect_t menu = start_menu_rect(info);
    if (!rect_contains_point(menu, x, y)) {
        return -1;
    }

    const int inner_y = y - (menu.y + 4);
    if (inner_y < 0) {
        return -1;
    }

    const int index = inner_y / START_MENU_ITEM_HEIGHT;
    if (index < 0 || index >= START_MENU_ITEMS) {
        return -1;
    }

    return index;
}

static const char *start_menu_item_text(int index) {
    switch (index) {
        case 0: return "Toggle Window 2";
        case 1: return "Clear Focused";
        case 2: return "Help";
        default: return "";
    }
}

static void draw_taskbar_clipped(
    int fb,
    const user_fb_info_t *info,
    const window_t windows[WINDOW_MAX],
    int focused,
    int start_menu_open,
    rect_t clip) {
    const rect_t bar = taskbar_rect(info);
    fb_fill_rect_clipped(fb, bar, clip, COLOR_TASKBAR_BG);
    fb_fill_rect_clipped(fb, rect_make(0, bar.y, info->width, 1), clip, COLOR_TASKBAR_BORDER);

    const rect_t start = taskbar_start_button_rect(info);
    fb_fill_rect_clipped(fb, start, clip, COLOR_TASKBAR_BUTTON_BG);
    fb_draw_string_clipped(fb, start.x + 10, start.y + 2, "Start",
        COLOR_MENU_TEXT, COLOR_TASKBAR_BUTTON_BG, clip);

    fb_draw_string_clipped(fb, start.x + start.width + 12, bar.y + 6, "Focused:",
        COLOR_TASKBAR_TEXT, COLOR_TASKBAR_BG, clip);

    if (focused >= 0 && focused < WINDOW_MAX && windows[focused].visible) {
        fb_draw_string_clipped(fb, start.x + start.width + 12 + 9 * FONT_WIDTH, bar.y + 6,
            windows[focused].title, COLOR_TASKBAR_TEXT, COLOR_TASKBAR_BG, clip);
    } else {
        fb_draw_string_clipped(fb, start.x + start.width + 12 + 9 * FONT_WIDTH, bar.y + 6,
            "(none)", COLOR_TASKBAR_TEXT, COLOR_TASKBAR_BG, clip);
    }

    if (!start_menu_open) {
        return;
    }

    const rect_t menu = start_menu_rect(info);
    fb_fill_rect_clipped(fb, menu, clip, COLOR_MENU_BG);
    fb_fill_rect_clipped(fb, rect_make(menu.x, menu.y, menu.width, 1), clip, COLOR_TASKBAR_BORDER);
    fb_fill_rect_clipped(fb, rect_make(menu.x, menu.y + menu.height - 1, menu.width, 1), clip, COLOR_TASKBAR_BORDER);
    fb_fill_rect_clipped(fb, rect_make(menu.x, menu.y, 1, menu.height), clip, COLOR_TASKBAR_BORDER);
    fb_fill_rect_clipped(fb, rect_make(menu.x + menu.width - 1, menu.y, 1, menu.height), clip, COLOR_TASKBAR_BORDER);

    for (int i = 0; i < START_MENU_ITEMS; ++i) {
        const rect_t item = rect_make(menu.x + 2, menu.y + 4 + i * START_MENU_ITEM_HEIGHT,
            menu.width - 4, START_MENU_ITEM_HEIGHT);
        fb_fill_rect_clipped(fb, item, clip, COLOR_MENU_BG);
        fb_draw_string_clipped(fb, item.x + 8, item.y + 2, start_menu_item_text(i),
            COLOR_MENU_TEXT, COLOR_MENU_BG, clip);
    }
}

static void draw_cursor(int fb, int x, int y) {
    fb_fill_rect(fb, x, y, CURSOR_WIDTH, CURSOR_HEIGHT, 0x00FFFFFF);
    fb_fill_rect(fb, x + 2, y + 2, CURSOR_WIDTH - 4, CURSOR_HEIGHT - 4, 0x00000000);
}

static void redraw_region(
    int fb,
    const user_fb_info_t *info,
    const window_t windows[WINDOW_MAX],
    int focused,
    int start_menu_open,
    rect_t clip) {
    // Cursor is an overlay; when it moves, we restore the old cursor area by redrawing only the
    // impacted pixels (background + windows) inside a small dirty rectangle.
    clip = rect_clamp_to_screen(clip, info);
    if (clip.width <= 0 || clip.height <= 0) {
        return;
    }

    fb_fill_rect(fb, clip.x, clip.y, clip.width, clip.height, COLOR_DESKTOP_BG);

    for (int i = 0; i < WINDOW_MAX; ++i) {
        if (i == focused) {
            continue;
        }
        rect_t win_rect = rect_make(windows[i].x, windows[i].y, windows[i].width, windows[i].height);
        rect_t visible;
        if (rect_intersect(&win_rect, &clip, &visible)) {
            window_draw_clipped(fb, &windows[i], clip);
        }
    }
    if (focused >= 0 && focused < WINDOW_MAX) {
        rect_t win_rect = rect_make(windows[focused].x, windows[focused].y, windows[focused].width, windows[focused].height);
        rect_t visible;
        if (rect_intersect(&win_rect, &clip, &visible)) {
            window_draw_clipped(fb, &windows[focused], clip);
        }
    }

    draw_taskbar_clipped(fb, info, windows, focused, start_menu_open, clip);
}

static void redraw(
    int fb,
    const user_fb_info_t *info,
    const window_t windows[WINDOW_MAX],
    int focused,
    int start_menu_open,
    int cursor_x,
    int cursor_y) {
    fb_fill_rect(fb, 0, 0, info->width, info->height, COLOR_DESKTOP_BG);

    for (int i = 0; i < WINDOW_MAX; ++i) {
        if (i == focused) {
            continue;
        }
        window_draw(fb, &windows[i]);
    }
    if (focused >= 0 && focused < WINDOW_MAX) {
        window_draw(fb, &windows[focused]);
    }

    draw_taskbar_clipped(fb, info, windows, focused, start_menu_open,
        rect_make(0, 0, info->width, info->height));
    draw_cursor(fb, cursor_x, cursor_y);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const int fb = sys_open("/dev/fb0");
    if (fb < 0) {
        write_str("wm: open /dev/fb0 failed\n");
        return 1;
    }

    user_fb_info_t info;
    if (sys_fb_info(fb, &info) < 0) {
        write_str("wm: fb info failed\n");
        sys_close(fb);
        return 1;
    }

    int mouse_fd = sys_open("/dev/mouse0");

    window_t windows[WINDOW_MAX];
    for (int i = 0; i < WINDOW_MAX; ++i) {
        windows[i].text_length = 0;
        windows[i].focused = false;
        windows[i].visible = true;
        windows[i].counter = 0;
        for (int j = 0; j < WINDOW_TEXT_MAX; ++j) {
            windows[i].text[j] = '\0';
        }
    }

    windows[0].x = 60;
    windows[0].y = 60;
    windows[0].width = 420;
    windows[0].height = 240;
    windows[0].title = "Window 1";
    windows[0].focused = true;
    windows[0].visible = true;

    windows[1].x = 220;
    windows[1].y = 180;
    windows[1].width = 420;
    windows[1].height = 240;
    windows[1].title = "Window 2";
    windows[1].visible = true;

    int focused = 0;
    int cursor_x = 20;
    int cursor_y = 20;
    int drawn_cursor_x = cursor_x;
    int drawn_cursor_y = cursor_y;
    int start_menu_open = 0;
    int dragging = 0;
    int drag_window = -1;
    int drag_offset_x = 0;
    int drag_offset_y = 0;

    redraw(fb, &info, windows, focused, start_menu_open, cursor_x, cursor_y);
    write_str("wm: drag title bar; click Start; esc/q quit\n");

    for (;;) {
        int need_redraw = 0;
        int cursor_moved = 0;
        const int old_cursor_x = cursor_x;
        const int old_cursor_y = cursor_y;
        int had_input = 0;

        if (mouse_fd >= 0) {
            user_mouse_event_t events[8];
            const int bytes = sys_read(mouse_fd, events, (int)sizeof(events));
            if (bytes > 0) {
                had_input = 1;
                const int count = bytes / (int)sizeof(events[0]);
                for (int i = 0; i < count; ++i) {
                    cursor_x = clamp_int(cursor_x + (int)events[i].dx, 0, info.width - CURSOR_WIDTH);
                    cursor_y = clamp_int(cursor_y + (int)events[i].dy, 0, info.height - CURSOR_HEIGHT);

                    const unsigned int changed = events[i].changed;
                    const unsigned int buttons = events[i].buttons;

                    // Window dragging happens while the left button is held down.
                    if (dragging && (buttons & USER_MOUSE_BUTTON_LEFT) != 0u) {
                        if (drag_window >= 0 && drag_window < WINDOW_MAX && windows[drag_window].visible) {
                            const int desired_x = cursor_x - drag_offset_x;
                            const int desired_y = cursor_y - drag_offset_y;

                            int max_x = info.width - windows[drag_window].width;
                            int max_y = taskbar_top(&info) - windows[drag_window].height;
                            if (max_x < 0) {
                                max_x = 0;
                            }
                            if (max_y < 0) {
                                max_y = 0;
                            }

                            const int new_x = clamp_int(desired_x, 0, max_x);
                            const int new_y = clamp_int(desired_y, 0, max_y);

                            if (new_x != windows[drag_window].x || new_y != windows[drag_window].y) {
                                windows[drag_window].x = new_x;
                                windows[drag_window].y = new_y;
                                need_redraw = 1;
                            }
                        }
                    }

                    // Mouse press: click widgets, focus windows, toggle menu, start dragging.
                    if ((changed & USER_MOUSE_BUTTON_LEFT) != 0u
                        && (buttons & USER_MOUSE_BUTTON_LEFT) != 0u) {
                        const rect_t start_button = taskbar_start_button_rect(&info);
                        if (rect_contains_point(start_button, cursor_x, cursor_y)) {
                            start_menu_open = !start_menu_open;
                            dragging = 0;
                            drag_window = -1;
                            need_redraw = 1;
                            continue;
                        }

                        if (start_menu_open) {
                            const int item = start_menu_item_at(&info, cursor_x, cursor_y);
                            if (item >= 0) {
                                if (item == 0) {
                                    windows[1].visible = !windows[1].visible;
                                    if (!windows[1].visible && focused == 1) {
                                        focused = first_visible_window(windows);
                                    }
                                } else if (item == 1) {
                                    if (focused >= 0 && focused < WINDOW_MAX) {
                                        windows[focused].counter = 0;
                                        window_set_text(&windows[focused], "");
                                    }
                                } else if (item == 2) {
                                    if (focused >= 0 && focused < WINDOW_MAX) {
                                        window_set_text(&windows[focused],
                                            "Drag title bar to move.\n"
                                            "Click the button to increment.\n"
                                            "Start toggles Window 2.");
                                    }
                                }

                                start_menu_open = 0;
                                need_redraw = 1;
                                continue;
                            }

                            // Clicking outside the menu closes it.
                            start_menu_open = 0;
                            need_redraw = 1;
                        }

                        int clicked = -1;
                        if (focused >= 0 && focused < WINDOW_MAX
                            && window_contains(&windows[focused], cursor_x, cursor_y)) {
                            clicked = focused;
                        } else {
                            for (int w = 0; w < WINDOW_MAX; ++w) {
                                if (w == focused) {
                                    continue;
                                }
                                if (window_contains(&windows[w], cursor_x, cursor_y)) {
                                    clicked = w;
                                    break;
                                }
                            }
                        }

                        if (clicked >= 0) {
                            if (focused != clicked) {
                                focused = clicked;
                                need_redraw = 1;
                            }

                            const rect_t close_rect = window_close_button_rect(&windows[clicked]);
                            const rect_t widget_rect = window_widget_button_rect(&windows[clicked]);
                            const rect_t title_rect = window_titlebar_rect(&windows[clicked]);

                            if (rect_contains_point(close_rect, cursor_x, cursor_y)) {
                                windows[clicked].visible = 0;
                                dragging = 0;
                                drag_window = -1;
                                if (focused == clicked) {
                                    focused = first_visible_window(windows);
                                }
                                need_redraw = 1;
                            } else if (rect_contains_point(widget_rect, cursor_x, cursor_y)) {
                                windows[clicked].counter++;
                                need_redraw = 1;
                            } else if (rect_contains_point(title_rect, cursor_x, cursor_y)) {
                                dragging = 1;
                                drag_window = clicked;
                                drag_offset_x = cursor_x - windows[clicked].x;
                                drag_offset_y = cursor_y - windows[clicked].y;
                            }
                        }
                    }

                    // Mouse release ends a drag operation.
                    if ((changed & USER_MOUSE_BUTTON_LEFT) != 0u
                        && (buttons & USER_MOUSE_BUTTON_LEFT) == 0u) {
                        dragging = 0;
                        drag_window = -1;
                    }
                }

                if (focused < 0 || focused >= WINDOW_MAX || !windows[focused].visible) {
                    focused = first_visible_window(windows);
                }

                cursor_moved = (cursor_x != old_cursor_x) || (cursor_y != old_cursor_y);
            }
        }

        char c = '\0';
        const int read_count = sys_read(0, &c, 1);

        if (read_count < 0) {
            break;
        }
        if (read_count > 0) {
            had_input = 1;

            if (c == 27 || c == 'q') {
                break;
            }

            if (c == '1') {
                if (windows[0].visible) {
                    focused = 0;
                    need_redraw = 1;
                }
            } else if (c == '2') {
                if (windows[1].visible) {
                    focused = 1;
                    need_redraw = 1;
                }
            } else if (c == '\b') {
                if (focused >= 0 && focused < WINDOW_MAX && windows[focused].text_length > 0) {
                    windows[focused].text_length--;
                    windows[focused].text[windows[focused].text_length] = '\0';
                    need_redraw = 1;
                }
            } else if (c >= 32 && c <= 126) {
                if (focused >= 0 && focused < WINDOW_MAX && windows[focused].text_length + 1 < WINDOW_TEXT_MAX) {
                    windows[focused].text[windows[focused].text_length++] = c;
                    windows[focused].text[windows[focused].text_length] = '\0';
                    need_redraw = 1;
                }
            }
        }

        for (int i = 0; i < WINDOW_MAX; ++i) {
            windows[i].focused = (i == focused) && windows[i].visible;
        }

        if (need_redraw) {
            redraw(fb, &info, windows, focused, start_menu_open, cursor_x, cursor_y);
            drawn_cursor_x = cursor_x;
            drawn_cursor_y = cursor_y;
        } else if (cursor_moved) {
            // Restore old cursor area + prepare new cursor area in one go.
            rect_t old_rect = rect_make(drawn_cursor_x, drawn_cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT);
            rect_t new_rect = rect_make(cursor_x, cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT);
            rect_t dirty = rect_union(old_rect, new_rect);

            redraw_region(fb, &info, windows, focused, start_menu_open, dirty);
            draw_cursor(fb, cursor_x, cursor_y);

            drawn_cursor_x = cursor_x;
            drawn_cursor_y = cursor_y;
        }

        if (!had_input && !cursor_moved && !need_redraw) {
            sys_yield();
        }
    } // End of the code
    if (mouse_fd >= 0) {
        sys_close(mouse_fd);
    }
    sys_close(fb);
    return 0;
}
