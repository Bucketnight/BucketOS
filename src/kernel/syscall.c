// syscall.c: Syscall dispatcher and implementations (write/open/read/close + fb + mouse).

/*
 * Reading guide:
 * - Purpose: syscall.c: Syscall dispatcher and implementations (write/open/read/close + fb + mouse).
 * - Start reading at: syscall_reset_process
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/console.h"
#include "bucketos/framebuffer.h"
#include "bucketos/gdt.h"
#include "bucketos/mouse.h"
#include "bucketos/process.h"
#include "bucketos/scheduler.h"
#include "bucketos/string.h"
#include "bucketos/syscall.h"
#include "bucketos/terminal.h"
#include "bucketos/vfs.h"

enum {
    SYSCALL_PATH_MAX = 96,
    SYSCALL_WRITE_CHUNK = 64,
    SYSCALL_LIST_CHUNK = 512,
    SYSCALL_BLIT_CHUNK_PIXELS = 32
};

typedef struct {
    char *buffer;
    size_t size;
    size_t used;
    bool truncated;
} syscall_list_context_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t type;
} user_framebuffer_info_t;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t color;
} user_framebuffer_rect_t;

typedef struct {
    uint32_t dst_x;
    uint32_t dst_y;
    uint32_t width;
    uint32_t height;
    uintptr_t source;
    uint32_t source_stride;
    uint32_t format;
} user_framebuffer_blit_t;

enum {
    FB_FORMAT_XRGB8888 = 1
};

static uint32_t g_framebuffer_owner_pid;

static process_fd_t *syscall_fd_get(process_t *process, uint32_t fd) {
    if (process == 0 || fd >= PROCESS_FD_MAX || !process->fds[fd].in_use) {
        return 0;
    }

    return &process->fds[fd];
}

static bool syscall_framebuffer_available(void) {
    const framebuffer_info_t *const framebuffer = framebuffer_info();
    return framebuffer->available
        && framebuffer->type == 1u
        && (framebuffer->bpp == 24u || framebuffer->bpp == 32u);
}

static bool syscall_framebuffer_force_acquire(process_t *process) {
    if (process == 0 || !syscall_framebuffer_available()) {
        return false;
    }

    if (g_framebuffer_owner_pid != 0u && g_framebuffer_owner_pid != process->pid) {
        return false;
    }

    g_framebuffer_owner_pid = process->pid;
    terminal_set_framebuffer_lock(true);
    return true;
}

static void syscall_framebuffer_release_if_unused(process_t *process) {
    if (process == 0 || g_framebuffer_owner_pid != process->pid) {
        return;
    }

    for (uint32_t fd = 0; fd < PROCESS_FD_MAX; ++fd) {
        if (process->fds[fd].in_use && process->fds[fd].type == PROCESS_FD_FRAMEBUFFER) {
            return;
        }
    }

    g_framebuffer_owner_pid = 0u;
    terminal_set_framebuffer_lock(false);
}

static void syscall_framebuffer_force_release(process_t *process) {
    if (process == 0 || g_framebuffer_owner_pid != process->pid) {
        return;
    }

    g_framebuffer_owner_pid = 0u;
    terminal_set_framebuffer_lock(false);
}

static bool syscall_framebuffer_rect_valid(
    const framebuffer_info_t *framebuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    uint32_t end_x;
    uint32_t end_y;

    if (framebuffer == 0 || width == 0 || height == 0) {
        return false;
    }

    if (__builtin_add_overflow(x, width, &end_x) || __builtin_add_overflow(y, height, &end_y)) {
        return false;
    }

    return end_x <= framebuffer->width && end_y <= framebuffer->height;
}

static void syscall_list_collect(const char *name, bool is_dir, void *context) {
    syscall_list_context_t *const list = (syscall_list_context_t *)context;
    const char *const prefix = is_dir ? "dir  " : "file ";
    const size_t prefix_length = strlen(prefix);
    const size_t name_length = strlen(name);
    const size_t total = prefix_length + name_length + 1u;

    if (list->used + total + 1u > list->size) {
        list->truncated = true;
        return;
    }

    memcpy(list->buffer + list->used, prefix, prefix_length);
    list->used += prefix_length;
    memcpy(list->buffer + list->used, name, name_length);
    list->used += name_length;
    list->buffer[list->used++] = '\n';
    list->buffer[list->used] = '\0';
}

static uint32_t syscall_write(registers_t *regs) {
    process_t *const process = process_current();
    process_fd_t *const fd = syscall_fd_get(process, regs->ebx);
    const char *const text = (const char *)(uintptr_t)regs->ecx;
    const size_t length = (size_t)regs->edx;
    char buffer[SYSCALL_WRITE_CHUNK];
    size_t written = 0;

    if (fd == 0 || text == 0 || length == 0) {
        return length == 0 ? 0 : (uint32_t)-1;
    }

    while (written < length) {
        const size_t chunk = (length - written) < sizeof(buffer)
            ? (length - written)
            : sizeof(buffer);

        if (!process_copy_from_user(buffer, text + written, chunk)) {
            return (uint32_t)-1;
        }

        if (fd->type == PROCESS_FD_CONSOLE) {
            console_write(buffer, chunk);
        } else if (fd->type != PROCESS_FD_NULL) {
            return (uint32_t)-1;
        }

        written += chunk;
    }

    return (uint32_t)written;
}

static uint32_t syscall_exit(registers_t *regs) {
    process_t *const process = process_current();

    if (process != 0) {
        syscall_framebuffer_force_release(process);
        memset(process->fds, 0, sizeof(process->fds));
        process->regs = *regs;
        process_mark_exited(process, regs->ebx);
    }

    scheduler_yield();
    gdt_prepare_user_exit(regs, regs->ebx);
    return regs->ebx;
}

static uint32_t syscall_open(registers_t *regs) {
    process_t *const process = process_current();
    const char *const path = (const char *)(uintptr_t)regs->ebx;
    char kernel_path[SYSCALL_PATH_MAX];
    size_t length = 0;
    const char *data;
    size_t size;

    if (process == 0 || path == 0 || !process_copy_user_string(kernel_path, sizeof(kernel_path), path)) {
        return (uint32_t)-1;
    }

    while (kernel_path[length] != '\0') {
        ++length;
    }

    for (uint32_t fd = 3; fd < PROCESS_FD_MAX; ++fd) {
        if (process->fds[fd].in_use) {
            continue;
        }

        if (strcmp(kernel_path, "/dev/fb0") == 0) {
            if (!syscall_framebuffer_force_acquire(process)) {
                return (uint32_t)-1;
            }

            process->fds[fd].in_use = true;
            process->fds[fd].type = PROCESS_FD_FRAMEBUFFER;
            return fd;
        }

        if (strcmp(kernel_path, "/dev/mouse0") == 0) {
            process->fds[fd].in_use = true;
            process->fds[fd].type = PROCESS_FD_MOUSE;
            return fd;
        }

        if (strcmp(kernel_path, "/dev/console") == 0) {
            process->fds[fd].in_use = true;
            process->fds[fd].type = PROCESS_FD_CONSOLE;
            return fd;
        }

        if (strcmp(kernel_path, "/dev/null") == 0) {
            process->fds[fd].in_use = true;
            process->fds[fd].type = PROCESS_FD_NULL;
            return fd;
        }

        if (length == 0 || !vfs_read(kernel_path, &data, &size)) {
            return (uint32_t)-1;
        }

        process->fds[fd].in_use = true;
        process->fds[fd].type = PROCESS_FD_FILE;
        process->fds[fd].data = data;
        process->fds[fd].size = size;
        process->fds[fd].offset = 0;
        return fd;
    }

    return (uint32_t)-1;
}

static uint32_t syscall_read(registers_t *regs) {
    process_t *const process = process_current();
    process_fd_t *const fd = syscall_fd_get(process, regs->ebx);
    char *const buffer = (char *)(uintptr_t)regs->ecx;
    const size_t requested = (size_t)regs->edx;

    if (fd == 0 || buffer == 0) {
        return (uint32_t)-1;
    }

    if (fd->type == PROCESS_FD_CONSOLE) {
        char kernel_buffer[SYSCALL_WRITE_CHUNK];
        const size_t chunk = requested < sizeof(kernel_buffer) ? requested : sizeof(kernel_buffer);
        const size_t read_count = console_read(kernel_buffer, chunk);

        if (read_count == 0) {
            return 0;
        }

        if (!process_copy_to_user(buffer, kernel_buffer, read_count)) {
            return (uint32_t)-1;
        }

        return (uint32_t)read_count;
    }

    if (fd->type == PROCESS_FD_NULL) {
        return 0;
    }

    if (fd->type == PROCESS_FD_FILE) {
        const size_t remaining = fd->size - fd->offset;
        const size_t count = requested < remaining ? requested : remaining;

        if (count != 0 && !process_copy_to_user(buffer, fd->data + fd->offset, count)) {
            return (uint32_t)-1;
        }

        fd->offset += count;
        return (uint32_t)count;
    }

    if (fd->type == PROCESS_FD_FRAMEBUFFER) {
        return (uint32_t)-1;
    }

    if (fd->type == PROCESS_FD_MOUSE) {
        mouse_event_t events[8];
        const size_t max_events = requested / sizeof(events[0]);
        const size_t event_count = mouse_read_events(events,
            max_events < 8u ? max_events : 8u);

        if (event_count == 0) {
            return 0;
        }

        const size_t bytes = event_count * sizeof(events[0]);
        if (!process_copy_to_user(buffer, events, bytes)) {
            return (uint32_t)-1;
        }

        return (uint32_t)bytes;
    }

    return (uint32_t)-1;
}

static uint32_t syscall_close(registers_t *regs) {
    process_t *const process = process_current();
    process_fd_t *const fd = syscall_fd_get(process, regs->ebx);

    if (fd == 0 || regs->ebx < 3u) {
        return (uint32_t)-1;
    }

    const process_fd_type_t type = fd->type;
    memset(fd, 0, sizeof(*fd));
    if (type == PROCESS_FD_FRAMEBUFFER) {
        syscall_framebuffer_release_if_unused(process);
    }
    return 0;
}

static uint32_t syscall_yield(registers_t *regs) {
    process_t *const process = process_current();

    if (process != 0) {
        process->regs = *regs;
    }

    scheduler_yield();
    return 0;
}

static uint32_t syscall_list(registers_t *regs) {
    const char *const path = (const char *)(uintptr_t)regs->ebx;
    char *const user_buffer = (char *)(uintptr_t)regs->ecx;
    const size_t user_buffer_size = (size_t)regs->edx;
    char kernel_path[SYSCALL_PATH_MAX];
    char kernel_buffer[SYSCALL_LIST_CHUNK];
    syscall_list_context_t context;

    if (path == 0 || user_buffer == 0 || user_buffer_size == 0) {
        return (uint32_t)-1;
    }

    if (!process_copy_user_string(kernel_path, sizeof(kernel_path), path)) {
        return (uint32_t)-1;
    }

    memset(&context, 0, sizeof(context));
    context.buffer = kernel_buffer;
    context.size = sizeof(kernel_buffer);
    kernel_buffer[0] = '\0';

    if (!vfs_list(kernel_path, syscall_list_collect, &context)) {
        return (uint32_t)-1;
    }

    if (context.used + 1u > user_buffer_size) {
        return (uint32_t)-1;
    }

    if (!process_copy_to_user(user_buffer, kernel_buffer, context.used + 1u)) {
        return (uint32_t)-1;
    }

    return (uint32_t)context.used;
}

static uint32_t syscall_clear(registers_t *regs) {
    (void)regs;
    console_clear();
    return 0;
}

static uint32_t syscall_fb_info(registers_t *regs) {
    process_t *const process = process_current();
    process_fd_t *const fd = syscall_fd_get(process, regs->ebx);
    user_framebuffer_info_t info;
    user_framebuffer_info_t *const user_info = (user_framebuffer_info_t *)(uintptr_t)regs->ecx;
    const framebuffer_info_t *const framebuffer = framebuffer_info();

    if (process == 0 || fd == 0 || fd->type != PROCESS_FD_FRAMEBUFFER) {
        return (uint32_t)-1;
    }

    if (g_framebuffer_owner_pid != process->pid || !syscall_framebuffer_available()) {
        return (uint32_t)-1;
    }

    memset(&info, 0, sizeof(info));
    info.width = framebuffer->width;
    info.height = framebuffer->height;
    info.pitch = framebuffer->pitch;
    info.bpp = framebuffer->bpp;
    info.type = framebuffer->type;

    if (user_info == 0 || !process_copy_to_user(user_info, &info, sizeof(info))) {
        return (uint32_t)-1;
    }

    return 0;
}

static uint32_t syscall_fb_putpixel(registers_t *regs) {
    process_t *const process = process_current();
    process_fd_t *const fd = syscall_fd_get(process, regs->ebx);
    const uint32_t x = regs->ecx;
    const uint32_t y = regs->edx;
    const uint32_t color = regs->esi;
    const framebuffer_info_t *const framebuffer = framebuffer_info();

    if (process == 0 || fd == 0 || fd->type != PROCESS_FD_FRAMEBUFFER) {
        return (uint32_t)-1;
    }

    if (g_framebuffer_owner_pid != process->pid || !syscall_framebuffer_available()) {
        return (uint32_t)-1;
    }

    if (x >= framebuffer->width || y >= framebuffer->height) {
        return (uint32_t)-1;
    }

    framebuffer_put_pixel(x, y, color);
    return 0;
}

static uint32_t syscall_fb_fill_rect(registers_t *regs) {
    process_t *const process = process_current();
    process_fd_t *const fd = syscall_fd_get(process, regs->ebx);
    const user_framebuffer_rect_t *const rect_user = (const user_framebuffer_rect_t *)(uintptr_t)regs->ecx;
    user_framebuffer_rect_t rect;
    const framebuffer_info_t *const framebuffer = framebuffer_info();

    if (process == 0 || fd == 0 || fd->type != PROCESS_FD_FRAMEBUFFER) {
        return (uint32_t)-1;
    }

    if (g_framebuffer_owner_pid != process->pid || !syscall_framebuffer_available()) {
        return (uint32_t)-1;
    }

    if (rect_user == 0 || !process_copy_from_user(&rect, rect_user, sizeof(rect))) {
        return (uint32_t)-1;
    }

    if (!syscall_framebuffer_rect_valid(framebuffer, rect.x, rect.y, rect.width, rect.height)) {
        return (uint32_t)-1;
    }

    framebuffer_draw_rect(rect.x, rect.y, rect.width, rect.height, rect.color);
    return 0;
}

static uint32_t syscall_fb_blit(registers_t *regs) {
    process_t *const process = process_current();
    process_fd_t *const fd = syscall_fd_get(process, regs->ebx);
    const user_framebuffer_blit_t *const blit_user = (const user_framebuffer_blit_t *)(uintptr_t)regs->ecx;
    user_framebuffer_blit_t blit;
    const framebuffer_info_t *const framebuffer = framebuffer_info();
    uint32_t pixels[SYSCALL_BLIT_CHUNK_PIXELS];

    if (process == 0 || fd == 0 || fd->type != PROCESS_FD_FRAMEBUFFER) {
        return (uint32_t)-1;
    }

    if (g_framebuffer_owner_pid != process->pid || !syscall_framebuffer_available()) {
        return (uint32_t)-1;
    }

    if (blit_user == 0 || !process_copy_from_user(&blit, blit_user, sizeof(blit))) {
        return (uint32_t)-1;
    }

    if (blit.format != FB_FORMAT_XRGB8888) {
        return (uint32_t)-1;
    }

    if (!syscall_framebuffer_rect_valid(framebuffer, blit.dst_x, blit.dst_y, blit.width, blit.height)) {
        return (uint32_t)-1;
    }

    if (blit.source == 0u || blit.source_stride < blit.width * sizeof(uint32_t)) {
        return (uint32_t)-1;
    }

    for (uint32_t row = 0; row < blit.height; ++row) {
        const uintptr_t row_source = blit.source + (uintptr_t)row * blit.source_stride;

        for (uint32_t col = 0; col < blit.width; col += SYSCALL_BLIT_CHUNK_PIXELS) {
            const uint32_t chunk_pixels = (blit.width - col) < SYSCALL_BLIT_CHUNK_PIXELS
                ? (blit.width - col)
                : SYSCALL_BLIT_CHUNK_PIXELS;

            const void *const user_pixels =
                (const void *)(row_source + (uintptr_t)col * sizeof(uint32_t));

            if (!process_copy_from_user(pixels, user_pixels, chunk_pixels * sizeof(uint32_t))) {
                return (uint32_t)-1;
            }

            for (uint32_t index = 0; index < chunk_pixels; ++index) {
                framebuffer_put_pixel(blit.dst_x + col + index, blit.dst_y + row, pixels[index]);
            }
        }
    }

    return 0;
}

void syscall_reset_process(void) {
    process_t *const process = process_current();
    process_reset_fds(process);
}

uint32_t syscall_dispatch(registers_t *regs) {
    switch (regs->eax) {
        case SYSCALL_WRITE:
            return syscall_write(regs);
        case SYSCALL_EXIT:
            return syscall_exit(regs);
        case SYSCALL_OPEN:
            return syscall_open(regs);
        case SYSCALL_READ:
            return syscall_read(regs);
        case SYSCALL_CLOSE:
            return syscall_close(regs);
        case SYSCALL_YIELD:
            return syscall_yield(regs);
        case SYSCALL_LIST:
            return syscall_list(regs);
        case SYSCALL_CLEAR:
            return syscall_clear(regs);
        case SYSCALL_FB_INFO:
            return syscall_fb_info(regs);
        case SYSCALL_FB_PUTPIXEL:
            return syscall_fb_putpixel(regs);
        case SYSCALL_FB_FILL_RECT:
            return syscall_fb_fill_rect(regs);
        case SYSCALL_FB_BLIT:
            return syscall_fb_blit(regs);
        default:
            return (uint32_t)-1;
    }
}
