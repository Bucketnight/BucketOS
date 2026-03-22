#include "bucketos/memory.h"
#include "bucketos/pit.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/shell.h"
#include "bucketos/string.h"
#include "bucketos/terminal.h"

enum {
    SHELL_BUFFER_SIZE = 128
};

static char g_buffer[SHELL_BUFFER_SIZE];
static size_t g_length;

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

    if (strcmp(g_buffer, "help") == 0) {
        print_line("commands: help about clear meminfo ticks shutdown");
    } else if (strcmp(g_buffer, "about") == 0) {
        print_line("v0.0.1");
    } else if (strcmp(g_buffer, "clear") == 0) {
        terminal_clear();
    } else if (strcmp(g_buffer, "meminfo") == 0) {
        print_memory_info();
    } else if (strcmp(g_buffer, "ticks") == 0) {
        print_string("ticks: ");
        print_uint32(pit_ticks());
        print_line("");
    } else if (strcmp(g_buffer, "shutdown") == 0) {
        print_line("powering off");

        outw(0x604, 0x2000);

        __asm__ volatile ("cli");
        for (;;) {
            cpu_halt();
        }
    } else {
        print_string("unknown command: ");
        print_line(g_buffer);
    }

    g_length = 0;
    shell_prompt();
}

void shell_initialize(void) {
    g_length = 0;
}

void shell_prompt(void) {
    print_string("\nbucket> ");
}

void shell_handle_char(char c) {
    if (c == '\n') {
        terminal_put_char('\n');
        shell_execute();
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
