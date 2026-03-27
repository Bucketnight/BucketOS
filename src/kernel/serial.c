#include "bucketos/config.h"
#include "bucketos/ports.h"
#include "bucketos/serial.h"

enum {
    SERIAL_DATA = 0,
    SERIAL_INTERRUPT_ENABLE = 1,
    SERIAL_FIFO_CONTROL = 2,
    SERIAL_LINE_CONTROL = 3,
    SERIAL_MODEM_CONTROL = 4,
    SERIAL_LINE_STATUS = 5
};

static bool g_serial_ready;

static uint16_t serial_port(void) {
    return (uint16_t)CONFIG_SERIAL_COM1_BASE;
}

void serial_initialize(void) {
#if CONFIG_SERIAL_LOG
    const uint16_t port = serial_port();

    outb(port + SERIAL_INTERRUPT_ENABLE, 0x00);
    outb(port + SERIAL_LINE_CONTROL, 0x80);
    outb(port + SERIAL_DATA, 0x03);
    outb(port + SERIAL_INTERRUPT_ENABLE, 0x00);
    outb(port + SERIAL_LINE_CONTROL, 0x03);
    outb(port + SERIAL_FIFO_CONTROL, 0xC7);
    outb(port + SERIAL_MODEM_CONTROL, 0x0B);

    g_serial_ready = true;
#else
    g_serial_ready = false;
#endif
}

bool serial_is_ready(void) {
    return g_serial_ready;
}

void serial_write_char(char c) {
#if CONFIG_SERIAL_LOG
    if (!g_serial_ready) {
        return;
    }

    const uint16_t port = serial_port();
    if (c == '\n') {
        serial_write_char('\r');
    }

    while ((inb(port + SERIAL_LINE_STATUS) & 0x20u) == 0u) {
    }

    outb(port + SERIAL_DATA, (uint8_t)c);
#else
    (void)c;
#endif
}

void serial_write(const char *text) {
    while (*text != '\0') {
        serial_write_char(*text++);
    }
}

void serial_write_line(const char *text) {
    serial_write(text);
    serial_write_char('\n');
}
