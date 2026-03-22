#include "bucketos/print.h"
#include "bucketos/serial.h"
#include "bucketos/terminal.h"

static void print_uint_recursive(uint32_t value) {
    if (value >= 10) {
        print_uint_recursive(value / 10);
    }
    print_char((char)('0' + (value % 10)));
}

void print_char(char c) {
    terminal_put_char(c);
    serial_write_char(c);
}

void print_string(const char *text) {
    while (*text != '\0') {
        print_char(*text++);
    }
}

void print_line(const char *text) {
    print_string(text);
    print_char('\n');
}

void print_uint32(uint32_t value) {
    print_uint_recursive(value);
}
void print_hex32(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";

    print_string("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        print_char(digits[(value >> shift) & 0xFu]);
    }
}

void print_hex64(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";

    print_string("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        print_char(digits[(value >> shift) & 0xFu]);
    }
}


void print_banner(void) {
    terminal_set_color(0x0A);
    print_line("BucketKernel");
    terminal_set_color(0x0F);
    print_line("small x86 kernel foundation");
    print_line("");
}
