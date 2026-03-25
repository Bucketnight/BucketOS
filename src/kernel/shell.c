#include "bucketos/memory.h"
#include "bucketos/panic.h"
#include "bucketos/pit.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/shell.h"
#include "bucketos/string.h"
#include "bucketos/terminal.h"
#include "bucketos/framebuffer.h"
#include "bucketos/usertest.h"
#include "bucketos/vfs.h"

enum {
    SHELL_BUFFER_SIZE = 128
};

static char g_buffer[SHELL_BUFFER_SIZE];
static size_t g_length;
static bool g_rect_mode;
static bool g_rect_exit_requested;
static bool g_command_ready;
static void shell_run_rect_demo(void);
static void shell_execute(void);

static char *skip_spaces(char *text) {
    while (*text == ' ') {
        ++text;
    }
    return text;
}

static void print_fs_entry(const char *name, bool is_dir, void *context) {
    (void)context;
    print_string(is_dir ? "dir  " : "file ");
    print_line(name);
}

static void shell_list_path(const char *path) {
    if (!vfs_list(path, print_fs_entry, 0)) {
        const char *data;
        size_t size;
        if (vfs_read(path, &data, &size)) {
            (void)data;
            (void)size;
            print_fs_entry(path, false, 0);
            return;
        }
        print_line("ls: path not found");
    }
}

static void shell_cat_file(const char *path) {
    const char *data;
    size_t size;

    if (!vfs_read(path, &data, &size)) {
        print_line("cat: file not found");
        return;
    }

    if (vfs_is_dir(path)) {
        print_line("cat: path is a directory");
        return;
    }

    (void)size;
    print_line(data);
}


static const char *memory_type_name(uint32_t type) {
    switch (type)
    {
    case 1: return "usable";
    case 2: return "reserved";
    case 3: return "acpi claimable";
    case 4: return "acpi nvs";
    case 5: return "bad memory";
    default: return "unknown";
    }
}

static void print_memory_info(void) {
    const memory_info_t *info = memory_info();

    print_string("mem lower kib: ");
    print_uint32(info->mem_lower_kib);
    print_line("");

    print_string("mem upper kib: ");
    print_uint32(info->mem_upper_kib);
    print_line("");

    print_string("usable bytes: ");
    print_hex32((uint32_t)(info->usable_bytes & 0xFFFFFFFFu));
    print_line("");

    print_string("heap start: ");
    print_hex32((uint32_t)info->heap_start);
    print_line("");

    print_string("heap end: ");
    print_hex32((uint32_t)info->heap_end);
    print_line("");

    print_line("memory regions:");
    for (size_t i = 0; i < info->region_count; ++i) {
        print_string("  base=");
        print_hex64(info->regions[i].base);
        print_string(" len=");
        print_hex64(info->regions[i].length);
        print_string(" type=");
        print_string(memory_type_name(info->regions[i].type));
        print_line("");
    }
}

static void shell_execute(void) {
    g_buffer[g_length] = '\0';

    if (g_length == 0) {
        shell_prompt();
        return;
    }

    char *command = g_buffer;
    char *args = command;
    while (*args != '\0' && *args != ' ') {
        ++args;
    }

    if (*args != '\0') {
        *args++ = '\0';
        args = skip_spaces(args);
    }

    if (strcmp(command, "help") == 0) {
        print_line("commands: help about clear meminfo ticks panic shutdown reboot rect usertest");
        print_line("fs: ls cat mkdir touch write");
        print_line("note: / mounts ramfs, /dev mounts devfs");
    } else if (strcmp(command, "about") == 0) {
        print_line("v0.0.1");
    } else if (strcmp(command, "clear") == 0) {
        terminal_clear();
    } else if (strcmp(command, "meminfo") == 0) {
        print_memory_info();
    } else if (strcmp(command, "reboot") == 0) {
        print_string("rebooting...");
        
        while ((inb(0x64) & 0x02u) != 0u) {
        }

        outb(0x64, 0xFE);

        for (;;) {
            cpu_halt();
        }
    } else if (strcmp(command, "ticks") == 0) {
        print_string("ticks: ");
        print_uint32(pit_ticks());
        print_line("");
    } else if (strcmp(command, "panic") == 0) {
        panic("panic command requested from shell");
    } else if (strcmp(command, "rect") == 0) {
        shell_run_rect_demo();
    } else if (strcmp(command, "usertest") == 0) {
        print_line("entering ring 3...");
        print_string("user exit: ");
        print_uint32(usertest_run());
        print_line("");
    } else if (strcmp(command, "shutdown") == 0) {
        print_line("powering off");

        outw(0x604, 0x2000);

        __asm__ volatile ("cli");
        for (;;) {
            cpu_halt();
        }
    } else if (strcmp(command, "ls") == 0) {
        shell_list_path((*args != '\0') ? args : "/");
    } else if (strcmp(command, "cat") == 0) {
        if (*args == '\0') {
            print_line("cat: missing path");
        } else {
            shell_cat_file(args);
        }
    } else if (strcmp(command, "mkdir") == 0) {
        if (*args == '\0') {
            print_line("mkdir: missing path");
        } else if (!vfs_mkdir(args)) {
            print_line("mkdir: failed");
        }
    } else if (strcmp(command, "touch") == 0) {
        if (*args == '\0') {
            print_line("touch: missing path");
        } else if (!vfs_touch(args)) {
            print_line("touch: failed");
        }
    } else if (strcmp(command, "write") == 0) {
        char *path = args;
        while (*args != '\0' && *args != ' ') {
            ++args;
        }

        if (*path == '\0') {
            print_line("write: missing path");
        } else {
            char *data = args;
            if (*data != '\0') {
                *data++ = '\0';
                data = skip_spaces(data);
            }

            if (!vfs_write(path, data)) {
                print_line("write: failed");
            }
        }
    } else {
        print_string("unknown command: ");
        print_line(command);
    }

    g_length = 0;
    shell_prompt();
}

void shell_initialize(void) {
    g_length = 0;
    g_rect_mode = false;
    g_rect_exit_requested = false;
    g_command_ready = false;
}

void shell_prompt(void) {
    print_string("\nbucket> ");
}

void shell_handle_char(char c) {
    if (c == '\n') {
        terminal_put_char('\n');
        g_command_ready = true;
        return;
    }

    if (g_rect_mode) {
        return;
    }

    if (c == '\b') {
        if (g_length == 0) {
            return;
        }

        --g_length;
        terminal_put_char('\b');
        return;
    }

    if (c == '\t') {
        return;
    }

    if (g_length + 1 >= SHELL_BUFFER_SIZE) {
        return;
    }

    g_buffer[g_length++] = c;
    terminal_put_char(c);
}

bool shell_has_pending_command(void) {
    return g_command_ready;
}

void shell_run_pending_command(void) {
    if (!g_command_ready) {
        return;
    }

    g_command_ready = false;
    shell_execute();
}

void shell_request_stop(void) {
    g_rect_exit_requested = true;
}
static void shell_run_rect_demo(void) {
    g_rect_mode = true;
    g_rect_exit_requested = false;

    print_line("rect demo: press Ctrl+C to stop");

    framebuffer_fill(0x00121824u);
    framebuffer_draw_rect(100, 80, 320, 180, 0x00FF0000u);
    framebuffer_draw_rect(160, 140, 180, 100, 0x0000FF00u);
    framebuffer_draw_rect(220, 200, 120, 60, 0x000000FFu);

    while (!g_rect_exit_requested) {
        cpu_halt();
    }

    g_rect_mode = false;
    g_rect_exit_requested = false;
    framebuffer_fill(0x00121824u);
}
