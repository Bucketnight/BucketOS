// console.c: Kernel console implementation (input buffer + output to terminal/serial).

#include "bucketos/console.h"
#include "bucketos/print.h"
#include "bucketos/process.h"
#include "bucketos/terminal.h"

enum {
    CONSOLE_INPUT_CAPACITY = 256
};

static char g_console_input[CONSOLE_INPUT_CAPACITY];
static size_t g_console_head;
static size_t g_console_tail;
static size_t g_console_count;

void console_initialize(void) {
    g_console_head = 0;
    g_console_tail = 0;
    g_console_count = 0;
}

bool console_input_is_active(void) {
    const process_t *const process = process_current();
    return process != 0 && process->state == PROCESS_STATE_RUNNING;
}

void console_push_input(char c) {
    if (g_console_count >= CONSOLE_INPUT_CAPACITY) {
        return;
    }

    g_console_input[g_console_tail] = c;
    g_console_tail = (g_console_tail + 1u) % CONSOLE_INPUT_CAPACITY;
    ++g_console_count;

    if (c == '\n' || c == '\b') {
        terminal_put_char(c);
    } else {
        print_char(c);
    }
}

size_t console_read(char *buffer, size_t size) {
    size_t read_count = 0;

    while (read_count < size && g_console_count > 0) {
        buffer[read_count++] = g_console_input[g_console_head];
        g_console_head = (g_console_head + 1u) % CONSOLE_INPUT_CAPACITY;
        --g_console_count;
    }

    return read_count;
}

void console_write(const char *data, size_t size) {
    for (size_t index = 0; index < size; ++index) {
        print_char(data[index]);
    }
}

void console_clear(void) {
    terminal_clear();
}
