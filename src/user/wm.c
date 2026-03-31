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

#define WALLPAPER_PATH "/usr/share/wallpaper.bgra"

enum {
    FONT_WIDTH = 8,
    FONT_HEIGHT = 16,
    WINDOW_TEXT_MAX = 4096,
    WINDOW_MAX = 3,
    WALLPAPER_WIDTH = 786,
    WALLPAPER_HEIGHT = 103,
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

typedef enum {
    WINDOW_ROLE_GENERIC = 0,
    WINDOW_ROLE_VIEWER = 1,
    WINDOW_ROLE_FILES = 2,
    WINDOW_ROLE_NOTEPAD = 3
} window_role_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char *title;
    char text[WINDOW_TEXT_MAX];
    int text_length;
    int text_cursor;
    int focused;
    int visible;
    int counter;
    window_role_t role;
} window_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} rect_t;

enum {
    DIRTY_RECTS_MAX = 16
};

typedef struct {
    rect_t rects[DIRTY_RECTS_MAX];
    int count;
} dirty_list_t;

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

// Cached wallpaper pixels (generated from logo.png at build time).
static unsigned int g_wallpaper_pixels[WALLPAPER_WIDTH * WALLPAPER_HEIGHT];
static int g_wallpaper_loaded;

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

static void dirty_reset(dirty_list_t *dirty) {
    if (dirty == 0) {
        return;
    }
    dirty->count = 0;
}

static void dirty_add(dirty_list_t *dirty, rect_t rect, const user_fb_info_t *info) {
    if (dirty == 0) {
        return;
    }

    rect = rect_clamp_to_screen(rect, info);
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }

    // Cheap merge: if it overlaps an existing rect, union it to keep the list small.
    for (int i = 0; i < dirty->count; ++i) {
        rect_t overlap;
        if (rect_intersect(&dirty->rects[i], &rect, &overlap)) {
            dirty->rects[i] = rect_union(dirty->rects[i], rect);
            return;
        }
    }

    if (dirty->count < DIRTY_RECTS_MAX) {
        dirty->rects[dirty->count++] = rect;
    } else {
        dirty->rects[0] = rect_union(dirty->rects[0], rect);
    }
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

static rect_t window_body_rect(const window_t *window) {
    return rect_make(window->x + 2, window->y + 24, window->width - 4, window->height - 26);
}

static rect_t window_content_rect(const window_t *window) {
    rect_t body = window_body_rect(window);
    if (window->role == WINDOW_ROLE_NOTEPAD) {
        const int footer = 28;
        body.height -= footer;
        if (body.height < 0) {
            body.height = 0;
        }
    }
    return body;
}

static rect_t window_notepad_clear_button_rect(const window_t *window) {
    return window_widget_button_rect(window);
}

static rect_t window_counter_label_rect(const window_t *window, unsigned int value) {
    char counter_text[32];
    build_counter_label(counter_text, value);
    const int label_width = strlen(counter_text) * FONT_WIDTH;
    const rect_t widget_rect = window_widget_button_rect(window);
    return rect_make(widget_rect.x + widget_rect.width + 12, widget_rect.y + 2, label_width, FONT_HEIGHT);
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
    window->text_cursor = length;
}

static void notepad_load(window_t *window, const char *path) {
    if (window == 0 || path == 0) {
        return;
    }

    const int fd = sys_open(path);
    if (fd < 0) {
        window_set_text(window, "");
        return;
    }

    int pos = 0;
    while (pos + 1 < WINDOW_TEXT_MAX) {
        const int read_count = sys_read(fd, window->text + pos, WINDOW_TEXT_MAX - 1 - pos);
        if (read_count <= 0) {
            break;
        }
        pos += read_count;
    }

    window->text[pos] = '\0';
    window->text_length = pos;
    window->text_cursor = pos;
    sys_close(fd);
}

static void window_text_cursor_xy(const window_t *window, int cursor, int *out_x, int *out_y) {
    if (window == 0 || out_x == 0 || out_y == 0) {
        return;
    }

    if (cursor < 0) {
        cursor = 0;
    }
    if (cursor > window->text_length) {
        cursor = window->text_length;
    }

    int x = window->x + 8;
    int y = window->y + 32;

    for (int i = 0; i < cursor; ++i) {
        const char c = window->text[i];
        if (c == '\n') {
            x = window->x + 8;
            y += FONT_HEIGHT;
        } else {
            x += FONT_WIDTH;
        }
    }

    *out_x = x;
    *out_y = y;
}

enum {
    FILES_PATH_MAX = 96,
    FILES_LIST_MAX = 1024,
    FILES_ENTRY_MAX = 48,
    FILES_NAME_MAX = 48,
    FILES_HEADER_LINES = 2
};

typedef struct {
    char cwd[FILES_PATH_MAX];
    int entry_count;
    struct {
        char name[FILES_NAME_MAX];
        int is_dir;
    } entries[FILES_ENTRY_MAX];
} file_explorer_t;

static void str_copy_limit(char *dst, int dst_size, const char *src) {
    if (dst == 0 || dst_size <= 0) {
        return;
    }

    int index = 0;
    if (src != 0) {
        while (src[index] != '\0' && index + 1 < dst_size) {
            dst[index] = src[index];
            ++index;
        }
    }

    dst[index] = '\0';
}

static int text_append(char *dst, int dst_size, int pos, const char *src) {
    if (dst == 0 || dst_size <= 0) {
        return pos;
    }

    if (pos < 0) {
        pos = 0;
    }

    if (src == 0) {
        dst[pos < dst_size ? pos : (dst_size - 1)] = '\0';
        return pos;
    }

    while (*src != '\0' && pos + 1 < dst_size) {
        dst[pos++] = *src++;
    }
    dst[pos < dst_size ? pos : (dst_size - 1)] = '\0';
    return pos;
}

static int path_is_root(const char *path) {
    return path != 0 && path[0] == '/' && path[1] == '\0';
}

static void path_parent(char *out, int out_size, const char *path) {
    if (out == 0 || out_size <= 0) {
        return;
    }

    if (path == 0 || path[0] != '/' || path_is_root(path)) {
        str_copy_limit(out, out_size, "/");
        return;
    }

    int length = strlen(path);
    while (length > 1 && path[length - 1] == '/') {
        --length;
    }

    int slash = length - 1;
    while (slash > 0 && path[slash] != '/') {
        --slash;
    }

    if (slash <= 0) {
        str_copy_limit(out, out_size, "/");
        return;
    }

    int pos = 0;
    while (pos + 1 < out_size && pos < slash) {
        out[pos] = path[pos];
        ++pos;
    }
    out[pos] = '\0';
}

static void path_join(char *out, int out_size, const char *base, const char *name) {
    if (out == 0 || out_size <= 0) {
        return;
    }

    if (base == 0 || base[0] != '/') {
        base = "/";
    }
    if (name == 0) {
        name = "";
    }

    int pos = 0;
    pos = text_append(out, out_size, pos, base);

    const int base_len = strlen(base);
    const int need_slash = base_len > 0 && base[base_len - 1] != '/' && !path_is_root(base);
    if (need_slash && pos + 1 < out_size) {
        out[pos++] = '/';
        out[pos] = '\0';
    }

    pos = text_append(out, out_size, pos, name);
}

static void viewer_show_file(window_t *viewer, const char *path) {
    if (viewer == 0 || path == 0) {
        return;
    }

    int pos = 0;
    pos = text_append(viewer->text, WINDOW_TEXT_MAX, pos, "file: ");
    pos = text_append(viewer->text, WINDOW_TEXT_MAX, pos, path);
    pos = text_append(viewer->text, WINDOW_TEXT_MAX, pos, "\n\n");

    const int fd = sys_open(path);
    if (fd < 0) {
        pos = text_append(viewer->text, WINDOW_TEXT_MAX, pos, "open failed\n");
        viewer->text_length = pos;
        return;
    }

    while (pos + 1 < WINDOW_TEXT_MAX) {
        const int read_count = sys_read(fd, viewer->text + pos, WINDOW_TEXT_MAX - 1 - pos);
        if (read_count <= 0) {
            break;
        }
        pos += read_count;
    }

    viewer->text[pos] = '\0';
    viewer->text_length = pos;
    sys_close(fd);
}

static void files_refresh(file_explorer_t *files, window_t *files_window) {
    if (files == 0 || files_window == 0) {
        return;
    }

    char list[FILES_LIST_MAX];
    if (sys_list(files->cwd, list, (int)sizeof(list)) < 0) {
        window_set_text(files_window, "files: list failed");
        files->entry_count = 0;
        return;
    }

    files->entry_count = 0;

    if (!path_is_root(files->cwd) && files->entry_count < FILES_ENTRY_MAX) {
        str_copy_limit(files->entries[files->entry_count].name, FILES_NAME_MAX, "..");
        files->entries[files->entry_count].is_dir = 1;
        files->entry_count++;
    }

    const char *cursor = list;
    while (*cursor != '\0' && files->entry_count < FILES_ENTRY_MAX) {
        int is_dir = 0;

        if (strncmp(cursor, "dir  ", 5) == 0) {
            is_dir = 1;
            cursor += 5;
        } else if (strncmp(cursor, "file ", 5) == 0) {
            is_dir = 0;
            cursor += 5;
        } else {
            while (*cursor != '\0' && *cursor != '\n') {
                ++cursor;
            }
            if (*cursor == '\n') {
                ++cursor;
            }
            continue;
        }

        char name[FILES_NAME_MAX];
        int name_len = 0;
        while (*cursor != '\0' && *cursor != '\n' && name_len + 1 < (int)sizeof(name)) {
            name[name_len++] = *cursor++;
        }
        name[name_len] = '\0';

        while (*cursor != '\0' && *cursor != '\n') {
            ++cursor;
        }
        if (*cursor == '\n') {
            ++cursor;
        }

        str_copy_limit(files->entries[files->entry_count].name, FILES_NAME_MAX, name);
        files->entries[files->entry_count].is_dir = is_dir;
        files->entry_count++;
    }

    char text[WINDOW_TEXT_MAX];
    int pos = 0;
    pos = text_append(text, (int)sizeof(text), pos, "path: ");
    pos = text_append(text, (int)sizeof(text), pos, files->cwd);
    pos = text_append(text, (int)sizeof(text), pos, "\n");
    pos = text_append(text, (int)sizeof(text), pos, "click: dir enter | file view\n");

    for (int index = 0; index < files->entry_count; ++index) {
        pos = text_append(text, (int)sizeof(text), pos, files->entries[index].is_dir ? "dir  " : "file ");
        pos = text_append(text, (int)sizeof(text), pos, files->entries[index].name);
        pos = text_append(text, (int)sizeof(text), pos, "\n");
    }

    window_set_text(files_window, text);
}

static int files_handle_click(
    file_explorer_t *files,
    window_t *files_window,
    window_t *viewer_window,
    int cursor_x,
    int cursor_y) {
    if (files == 0 || files_window == 0 || viewer_window == 0) {
        return 0;
    }

    const rect_t body = rect_make(files_window->x + 2, files_window->y + 24,
        files_window->width - 4, files_window->height - 26);

    if (!rect_contains_point(body, cursor_x, cursor_y)) {
        return 0;
    }

    const int text_start_y = files_window->y + 32;
    if (cursor_y < text_start_y) {
        return 0;
    }

    const int line = (cursor_y - text_start_y) / FONT_HEIGHT;
    const int index = line - FILES_HEADER_LINES;

    if (index < 0 || index >= files->entry_count) {
        return 0;
    }

    const char *const name = files->entries[index].name;

    char path[FILES_PATH_MAX];
    if (files->entries[index].is_dir) {
        if (strcmp(name, "..") == 0) {
            path_parent(path, (int)sizeof(path), files->cwd);
        } else {
            path_join(path, (int)sizeof(path), files->cwd, name);
        }

        str_copy_limit(files->cwd, (int)sizeof(files->cwd), path);
        files_refresh(files, files_window);
        return 1;
    }

    path_join(path, (int)sizeof(path), files->cwd, name);
    viewer_show_file(viewer_window, path);
    return 1;
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
    const rect_t body_rect = window_body_rect(window);
    const rect_t content_rect = window_content_rect(window);

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

        rect_t text_clip;
        if (rect_intersect(&content_rect, &clip, &text_clip)) {
            fb_draw_string_clipped(fb, window->x + 8, window->y + 32, line, body_fg, body_bg, text_clip);
        }
    }

    if (window->role == WINDOW_ROLE_GENERIC) {
        // Simple widget: a clickable button that increments a per-window counter.
        const rect_t widget_rect = window_widget_button_rect(window);
        fb_fill_rect_clipped(fb, widget_rect, clip, widget_bg);
        fb_draw_string_clipped(fb, widget_rect.x + 8, widget_rect.y + 2, "Click",
            0x00FFFFFF, widget_bg, clip);

        char counter_text[32];
        build_counter_label(counter_text, (unsigned int)window->counter);
        fb_draw_string_clipped(fb, widget_rect.x + widget_rect.width + 12, widget_rect.y + 2,
            counter_text, body_fg, body_bg, clip);
    }

    if (window->role == WINDOW_ROLE_NOTEPAD) {
        const rect_t clear_rect = window_notepad_clear_button_rect(window);
        fb_fill_rect_clipped(fb, clear_rect, clip, widget_bg);
        fb_draw_string_clipped(fb, clear_rect.x + 8, clear_rect.y + 2, "Clear",
            0x00FFFFFF, widget_bg, clip);

        // Caret is a simple 2px underline drawn at the insertion point.
        if (window->focused) {
            int caret_x;
            int caret_y;
            window_text_cursor_xy(window, window->text_cursor, &caret_x, &caret_y);
            const rect_t caret_rect = rect_make(caret_x, caret_y + FONT_HEIGHT - 2, FONT_WIDTH, 2);
            fb_fill_rect_clipped(fb, caret_rect, clip, 0x00FFD34D);
        }
    }
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

        const rect_t content_rect = window_content_rect(window);
        fb_draw_string_clipped(fb, window->x + 8, window->y + 32, line, body_fg, body_bg, content_rect);
    }

    if (window->role == WINDOW_ROLE_GENERIC) {
        const rect_t widget_rect = window_widget_button_rect(window);
        fb_fill_rect(fb, widget_rect.x, widget_rect.y, widget_rect.width, widget_rect.height, widget_bg);
        fb_draw_string(fb, widget_rect.x + 8, widget_rect.y + 2, "Click", 0x00FFFFFF, widget_bg);

        char counter_text[32];
        build_counter_label(counter_text, (unsigned int)window->counter);
        fb_draw_string(fb, widget_rect.x + widget_rect.width + 12, widget_rect.y + 2,
            counter_text, body_fg, body_bg);
    }

    if (window->role == WINDOW_ROLE_NOTEPAD) {
        const rect_t clear_rect = window_notepad_clear_button_rect(window);
        fb_fill_rect(fb, clear_rect.x, clear_rect.y, clear_rect.width, clear_rect.height, widget_bg);
        fb_draw_string(fb, clear_rect.x + 8, clear_rect.y + 2, "Clear", 0x00FFFFFF, widget_bg);

        if (window->focused) {
            int caret_x;
            int caret_y;
            window_text_cursor_xy(window, window->text_cursor, &caret_x, &caret_y);
            fb_fill_rect(fb, caret_x, caret_y + FONT_HEIGHT - 2, FONT_WIDTH, 2, 0x00FFD34D);
        }
    }
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
        case 0: return "Toggle Files";
        case 1: return "Clear Text";
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

static void wallpaper_load(void) {
    const int fd = sys_open(WALLPAPER_PATH);
    if (fd < 0) {
        g_wallpaper_loaded = 0;
        return;
    }

    unsigned char *const dst = (unsigned char *)g_wallpaper_pixels;
    const int total_bytes = WALLPAPER_WIDTH * WALLPAPER_HEIGHT * 4;
    int offset = 0;

    while (offset < total_bytes) {
        const int read_count = sys_read(fd, dst + offset, total_bytes - offset);
        if (read_count <= 0) {
            break;
        }
        offset += read_count;
    }

    sys_close(fd);
    g_wallpaper_loaded = (offset == total_bytes);
}

static rect_t wallpaper_rect(const user_fb_info_t *info) {
    int x = 0;
    int y = 0;

    if (info != 0) {
        x = (info->width - WALLPAPER_WIDTH) / 2;
        y = (taskbar_top(info) - WALLPAPER_HEIGHT) / 2;
    }

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }

    return rect_make(x, y, WALLPAPER_WIDTH, WALLPAPER_HEIGHT);
}

static void desktop_draw_background_clipped(int fb, const user_fb_info_t *info, rect_t clip) {
    fb_fill_rect(fb, clip.x, clip.y, clip.width, clip.height, COLOR_DESKTOP_BG);

    if (!g_wallpaper_loaded) {
        return;
    }

    const rect_t wallpaper = wallpaper_rect(info);
    rect_t visible;

    if (!rect_intersect(&wallpaper, &clip, &visible)) {
        return;
    }

    const int src_x = visible.x - wallpaper.x;
    const int src_y = visible.y - wallpaper.y;

    user_fb_blit_t blit;
    blit.dst_x = visible.x;
    blit.dst_y = visible.y;
    blit.width = visible.width;
    blit.height = visible.height;
    blit.source = &g_wallpaper_pixels[src_y * WALLPAPER_WIDTH + src_x];
    blit.source_stride = WALLPAPER_WIDTH * 4;
    blit.format = USER_FB_FORMAT_XRGB8888;
    sys_fb_blit(fb, &blit);
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

    desktop_draw_background_clipped(fb, info, clip);

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
    desktop_draw_background_clipped(fb, info, rect_make(0, 0, info->width, info->height));

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

    wallpaper_load();

    int mouse_fd = sys_open("/dev/mouse0");

    window_t windows[WINDOW_MAX];
    for (int i = 0; i < WINDOW_MAX; ++i) {
        windows[i].text_length = 0;
        windows[i].text_cursor = 0;
        windows[i].focused = false;
        windows[i].visible = true;
        windows[i].counter = 0;
        windows[i].role = WINDOW_ROLE_GENERIC;
        for (int j = 0; j < WINDOW_TEXT_MAX; ++j) {
            windows[i].text[j] = '\0';
        }
    }

    windows[0].x = 60;
    windows[0].y = 60;
    windows[0].width = 420;
    windows[0].height = 240;
    windows[0].title = "Notepad";
    windows[0].focused = true;
    windows[0].visible = true;
    windows[0].role = WINDOW_ROLE_NOTEPAD;
    notepad_load(&windows[0], "/home/notes.txt");

    windows[1].x = 520;
    windows[1].y = 60;
    windows[1].width = 440;
    windows[1].height = 240;
    windows[1].title = "Viewer";
    windows[1].visible = true;
    windows[1].role = WINDOW_ROLE_VIEWER;
    window_set_text(&windows[1], "Click a file in Files to preview it.");

    windows[2].x = 60;
    windows[2].y = 330;
    windows[2].width = 900;
    windows[2].height = 360;
    windows[2].title = "Files";
    windows[2].visible = true;
    windows[2].role = WINDOW_ROLE_FILES;

    file_explorer_t files;
    str_copy_limit(files.cwd, (int)sizeof(files.cwd), "/");
    files.entry_count = 0;
    files_refresh(&files, &windows[2]);

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

    for (;;) {
        int cursor_moved = 0;
        const int old_cursor_x = cursor_x;
        const int old_cursor_y = cursor_y;
        dirty_list_t dirty;
        dirty_reset(&dirty);
        int had_input = 0;
        const int old_focused = focused;

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
                                const rect_t old_rect = rect_make(
                                    windows[drag_window].x, windows[drag_window].y,
                                    windows[drag_window].width, windows[drag_window].height);
                                windows[drag_window].x = new_x;
                                windows[drag_window].y = new_y;
                                const rect_t new_rect = rect_make(
                                    windows[drag_window].x, windows[drag_window].y,
                                    windows[drag_window].width, windows[drag_window].height);
                                const rect_t moved = rect_union(old_rect, new_rect);
                                dirty_add(&dirty, moved, &info);
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
                            dirty_add(&dirty, taskbar_rect(&info), &info);
                            dirty_add(&dirty, start_menu_rect(&info), &info);
                            continue;
                        }

                        if (start_menu_open) {
                            const int item = start_menu_item_at(&info, cursor_x, cursor_y);
                            if (item >= 0) {
                                if (item == 0) {
                                    const rect_t files_rect = rect_make(
                                        windows[2].x, windows[2].y, windows[2].width, windows[2].height);
                                    windows[2].visible = !windows[2].visible;
                                    if (!windows[2].visible && focused == 2) {
                                        focused = first_visible_window(windows);
                                    }
                                    dirty_add(&dirty, files_rect, &info);
                                } else if (item == 1) {
                                    if (focused >= 0 && focused < WINDOW_MAX
                                        && (windows[focused].role == WINDOW_ROLE_GENERIC
                                            || windows[focused].role == WINDOW_ROLE_NOTEPAD)) {
                                        windows[focused].counter = 0;
                                        windows[focused].text_length = 0;
                                        windows[focused].text_cursor = 0;
                                        windows[focused].text[0] = '\0';
                                        dirty_add(&dirty, window_content_rect(&windows[focused]), &info);
                                    }
                                } else if (item == 2) {
                                    window_set_text(&windows[1],
                                        "Help:\n"
                                        "- Left click title bar to drag.\n"
                                        "- Start button toggles the menu.\n"
                                        "- Files lists the VFS tree.\n"
                                        "- Notepad is editable.\n"
                                        "- ESC exits the WM.");
                                    windows[1].visible = true;
                                    focused = 1;
                                    dirty_add(&dirty, rect_make(windows[1].x, windows[1].y, windows[1].width, windows[1].height), &info);
                                }

                                start_menu_open = 0;
                                dirty_add(&dirty, taskbar_rect(&info), &info);
                                dirty_add(&dirty, start_menu_rect(&info), &info);
                                continue;
                            }

                            // Clicking outside the menu closes it.
                            start_menu_open = 0;
                            dirty_add(&dirty, taskbar_rect(&info), &info);
                            dirty_add(&dirty, start_menu_rect(&info), &info);
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
                                const int prev_focus = focused;
                                focused = clicked;
                                if (prev_focus >= 0 && prev_focus < WINDOW_MAX && windows[prev_focus].visible) {
                                    dirty_add(&dirty, rect_make(windows[prev_focus].x, windows[prev_focus].y,
                                        windows[prev_focus].width, windows[prev_focus].height), &info);
                                }
                                dirty_add(&dirty, rect_make(windows[focused].x, windows[focused].y,
                                    windows[focused].width, windows[focused].height), &info);
                                dirty_add(&dirty, taskbar_rect(&info), &info);
                            }

                            const rect_t close_rect = window_close_button_rect(&windows[clicked]);
                            const rect_t widget_rect = window_widget_button_rect(&windows[clicked]);
                            const rect_t title_rect = window_titlebar_rect(&windows[clicked]);

                            if (rect_contains_point(close_rect, cursor_x, cursor_y)) {
                                const rect_t closed_rect = rect_make(
                                    windows[clicked].x, windows[clicked].y, windows[clicked].width, windows[clicked].height);
                                windows[clicked].visible = 0;
                                dragging = 0;
                                drag_window = -1;
                                if (focused == clicked) {
                                    focused = first_visible_window(windows);
                                }
                                dirty_add(&dirty, closed_rect, &info);
                                dirty_add(&dirty, taskbar_rect(&info), &info);
                            } else if (rect_contains_point(title_rect, cursor_x, cursor_y)) {
                                dragging = 1;
                                drag_window = clicked;
                                drag_offset_x = cursor_x - windows[clicked].x;
                                drag_offset_y = cursor_y - windows[clicked].y;
                            } else if (windows[clicked].role == WINDOW_ROLE_FILES) {
                                if (files_handle_click(&files, &windows[clicked], &windows[1], cursor_x, cursor_y)) {
                                    dirty_add(&dirty, window_body_rect(&windows[clicked]), &info);
                                    dirty_add(&dirty, window_body_rect(&windows[1]), &info);
                                }
                            } else if (windows[clicked].role == WINDOW_ROLE_GENERIC
                                && rect_contains_point(widget_rect, cursor_x, cursor_y)) {
                                const rect_t old_label =
                                    window_counter_label_rect(&windows[clicked], (unsigned int)windows[clicked].counter);
                                windows[clicked].counter++;
                                const rect_t new_label =
                                    window_counter_label_rect(&windows[clicked], (unsigned int)windows[clicked].counter);
                                const rect_t changed = rect_union(old_label, new_label);
                                dirty_add(&dirty, changed, &info);
                            } else if (windows[clicked].role == WINDOW_ROLE_NOTEPAD) {
                                const rect_t clear_rect = window_notepad_clear_button_rect(&windows[clicked]);
                                if (rect_contains_point(clear_rect, cursor_x, cursor_y)) {
                                    windows[clicked].text_length = 0;
                                    windows[clicked].text_cursor = 0;
                                    windows[clicked].text[0] = '\0';
                                    dirty_add(&dirty, window_content_rect(&windows[clicked]), &info);
                                    dirty_add(&dirty, clear_rect, &info);
                                }
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

            if (c == 27) {
                break;
            }

            if (c == '1') {
                if (windows[0].visible) {
                    const int prev = focused;
                    focused = 0;
                    if (prev != focused) {
                        if (prev >= 0 && prev < WINDOW_MAX) {
                            dirty_add(&dirty, rect_make(windows[prev].x, windows[prev].y, windows[prev].width, windows[prev].height), &info);
                        }
                        dirty_add(&dirty, rect_make(windows[focused].x, windows[focused].y, windows[focused].width, windows[focused].height), &info);
                        dirty_add(&dirty, taskbar_rect(&info), &info);
                    }
                }
            } else if (c == '2') {
                if (windows[1].visible) {
                    const int prev = focused;
                    focused = 1;
                    if (prev != focused) {
                        if (prev >= 0 && prev < WINDOW_MAX) {
                            dirty_add(&dirty, rect_make(windows[prev].x, windows[prev].y, windows[prev].width, windows[prev].height), &info);
                        }
                        dirty_add(&dirty, rect_make(windows[focused].x, windows[focused].y, windows[focused].width, windows[focused].height), &info);
                        dirty_add(&dirty, taskbar_rect(&info), &info);
                    }
                }
            } else if (c == '3') {
                if (windows[2].visible) {
                    const int prev = focused;
                    focused = 2;
                    if (prev != focused) {
                        if (prev >= 0 && prev < WINDOW_MAX) {
                            dirty_add(&dirty, rect_make(windows[prev].x, windows[prev].y, windows[prev].width, windows[prev].height), &info);
                        }
                        dirty_add(&dirty, rect_make(windows[focused].x, windows[focused].y, windows[focused].width, windows[focused].height), &info);
                        dirty_add(&dirty, taskbar_rect(&info), &info);
                    }
                }
            } else if (c == '\b') {
                if (focused >= 0 && focused < WINDOW_MAX
                    && (windows[focused].role == WINDOW_ROLE_GENERIC || windows[focused].role == WINDOW_ROLE_NOTEPAD)
                    && windows[focused].text_cursor > 0) {
                    const int old_cursor = windows[focused].text_cursor;
                    windows[focused].text_cursor--;
                    if (windows[focused].text_length > windows[focused].text_cursor) {
                        windows[focused].text_length = windows[focused].text_cursor;
                    }
                    windows[focused].text[windows[focused].text_length] = '\0';

                    int old_x;
                    int old_y;
                    int new_x;
                    int new_y;
                    window_text_cursor_xy(&windows[focused], old_cursor, &old_x, &old_y);
                    window_text_cursor_xy(&windows[focused], windows[focused].text_cursor, &new_x, &new_y);
                    dirty_add(&dirty, rect_make(new_x, new_y, FONT_WIDTH, FONT_HEIGHT), &info);
                    dirty_add(&dirty, rect_make(old_x, old_y, FONT_WIDTH, FONT_HEIGHT), &info);
                }
            } else if (c == '\n') {
                if (focused >= 0 && focused < WINDOW_MAX
                    && windows[focused].role == WINDOW_ROLE_NOTEPAD
                    && windows[focused].text_cursor + 1 < WINDOW_TEXT_MAX) {
                    const int old_cursor = windows[focused].text_cursor;
                    windows[focused].text[windows[focused].text_cursor++] = '\n';
                    windows[focused].text_length = windows[focused].text_cursor;
                    windows[focused].text[windows[focused].text_length] = '\0';

                    int old_x;
                    int old_y;
                    int new_x;
                    int new_y;
                    window_text_cursor_xy(&windows[focused], old_cursor, &old_x, &old_y);
                    window_text_cursor_xy(&windows[focused], windows[focused].text_cursor, &new_x, &new_y);
                    dirty_add(&dirty, rect_make(old_x, old_y, FONT_WIDTH, FONT_HEIGHT), &info);
                    dirty_add(&dirty, rect_make(new_x, new_y, FONT_WIDTH, FONT_HEIGHT), &info);
                }
            } else if (c >= 32 && c <= 126) {
                if (focused >= 0 && focused < WINDOW_MAX
                    && (windows[focused].role == WINDOW_ROLE_GENERIC || windows[focused].role == WINDOW_ROLE_NOTEPAD)
                    && windows[focused].text_cursor + 1 < WINDOW_TEXT_MAX) {
                    const int old_cursor = windows[focused].text_cursor;
                    windows[focused].text[windows[focused].text_cursor++] = c;
                    windows[focused].text_length = windows[focused].text_cursor;
                    windows[focused].text[windows[focused].text_length] = '\0';

                    int old_x;
                    int old_y;
                    int new_x;
                    int new_y;
                    window_text_cursor_xy(&windows[focused], old_cursor, &old_x, &old_y);
                    window_text_cursor_xy(&windows[focused], windows[focused].text_cursor, &new_x, &new_y);
                    dirty_add(&dirty, rect_make(old_x, old_y, FONT_WIDTH, FONT_HEIGHT), &info);
                    dirty_add(&dirty, rect_make(new_x, new_y, FONT_WIDTH, FONT_HEIGHT), &info);
                }
            }
        }

        for (int i = 0; i < WINDOW_MAX; ++i) {
            windows[i].focused = (i == focused) && windows[i].visible;
        }

        if (old_focused != focused) {
            if (old_focused >= 0 && old_focused < WINDOW_MAX) {
                dirty_add(&dirty, rect_make(windows[old_focused].x, windows[old_focused].y,
                    windows[old_focused].width, windows[old_focused].height), &info);
            }
            if (focused >= 0 && focused < WINDOW_MAX) {
                dirty_add(&dirty, rect_make(windows[focused].x, windows[focused].y,
                    windows[focused].width, windows[focused].height), &info);
            }
            dirty_add(&dirty, taskbar_rect(&info), &info);
        }

        if (cursor_moved) {
            dirty_add(&dirty, rect_make(drawn_cursor_x, drawn_cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT), &info);
            dirty_add(&dirty, rect_make(cursor_x, cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT), &info);
        }

        int cursor_needs_redraw = cursor_moved;
        const rect_t cursor_rect = rect_make(cursor_x, cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT);
        for (int i = 0; i < dirty.count; ++i) {
            redraw_region(fb, &info, windows, focused, start_menu_open, dirty.rects[i]);

            if (!cursor_needs_redraw) {
                rect_t visible;
                if (rect_intersect(&cursor_rect, &dirty.rects[i], &visible)) {
                    cursor_needs_redraw = 1;
                }
            }
        }

        if (cursor_needs_redraw) {
            draw_cursor(fb, cursor_x, cursor_y);
            drawn_cursor_x = cursor_x;
            drawn_cursor_y = cursor_y;
        }

        if (!had_input && !cursor_moved && dirty.count == 0) {
            sys_yield();
        }
    }

    if (mouse_fd >= 0) {
        sys_close(mouse_fd);
    }
    sys_close(fb);
    return 0;
}
