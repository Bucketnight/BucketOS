// mouse.c: PS/2 mouse driver (IRQ12 packet decode + ring buffer of events).

/*
 * Reading guide:
 * - Purpose: mouse.c: PS/2 mouse driver (IRQ12 packet decode + ring buffer of events).
 * - Start reading at: mouse_read_events
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/mouse.h"
#include "bucketos/ports.h"

enum {
    PS2_DATA_PORT = 0x60,
    PS2_STATUS_PORT = 0x64,
    PS2_COMMAND_PORT = 0x64,

    PS2_STATUS_OUTPUT_FULL = 0x01,
    PS2_STATUS_INPUT_FULL = 0x02,
    PS2_STATUS_AUX_DATA = 0x20,

    PS2_CMD_ENABLE_AUX = 0xA8,
    PS2_CMD_READ_COMMAND_BYTE = 0x20,
    PS2_CMD_WRITE_COMMAND_BYTE = 0x60,
    PS2_CMD_WRITE_AUX = 0xD4,

    PS2_MOUSE_SET_DEFAULTS = 0xF6,
    PS2_MOUSE_ENABLE_REPORTING = 0xF4,
    PS2_MOUSE_ACK = 0xFA
};

enum {
    MOUSE_QUEUE_CAPACITY = 64
};

static mouse_event_t g_queue[MOUSE_QUEUE_CAPACITY];
static size_t g_queue_head;
static size_t g_queue_tail;
static size_t g_queue_count;

static uint8_t g_packet[3];
static uint8_t g_packet_index;
static uint8_t g_last_buttons;

static void ps2_wait_input_clear(void) {
    for (uint32_t i = 0; i < 200000u; ++i) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) == 0u) {
            return;
        }
    }
}

static void ps2_wait_output_full(void) {
    for (uint32_t i = 0; i < 200000u; ++i) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) {
            return;
        }
    }
}

static void ps2_write_command(uint8_t command) {
    ps2_wait_input_clear();
    outb(PS2_COMMAND_PORT, command);
}

static void ps2_write_data(uint8_t data) {
    ps2_wait_input_clear();
    outb(PS2_DATA_PORT, data);
}

static uint8_t ps2_read_data(void) {
    ps2_wait_output_full();
    return inb(PS2_DATA_PORT);
}

static void mouse_write(uint8_t value) {
    ps2_write_command(PS2_CMD_WRITE_AUX);
    ps2_write_data(value);
}

static bool mouse_expect_ack(void) {
    return ps2_read_data() == PS2_MOUSE_ACK;
}

static void mouse_queue_push(mouse_event_t event) {
    if (g_queue_count >= MOUSE_QUEUE_CAPACITY) {
        return;
    }

    g_queue[g_queue_tail] = event;
    g_queue_tail = (g_queue_tail + 1u) % MOUSE_QUEUE_CAPACITY;
    ++g_queue_count;
}

size_t mouse_read_events(mouse_event_t *events, size_t max_events) {
    size_t read_count = 0;

    while (read_count < max_events && g_queue_count > 0) {
        events[read_count++] = g_queue[g_queue_head];
        g_queue_head = (g_queue_head + 1u) % MOUSE_QUEUE_CAPACITY;
        --g_queue_count;
    }

    return read_count;
}

void mouse_initialize(void) {
    g_queue_head = 0;
    g_queue_tail = 0;
    g_queue_count = 0;
    g_packet_index = 0;
    g_last_buttons = 0;

    ps2_write_command(PS2_CMD_ENABLE_AUX);

    ps2_write_command(PS2_CMD_READ_COMMAND_BYTE);
    uint8_t command_byte = ps2_read_data();
    command_byte |= 0x02u;
    command_byte |= 0x01u;
    ps2_write_command(PS2_CMD_WRITE_COMMAND_BYTE);
    ps2_write_data(command_byte);

    mouse_write(PS2_MOUSE_SET_DEFAULTS);
    (void)mouse_expect_ack();

    mouse_write(PS2_MOUSE_ENABLE_REPORTING);
    (void)mouse_expect_ack();
}

void mouse_handle_irq(void) {
    const uint8_t status = inb(PS2_STATUS_PORT);

    if ((status & PS2_STATUS_OUTPUT_FULL) == 0u) {
        return;
    }

    if ((status & PS2_STATUS_AUX_DATA) == 0u) {
        (void)inb(PS2_DATA_PORT);
        return;
    }

    const uint8_t data = inb(PS2_DATA_PORT);

    if (g_packet_index == 0u && (data & 0x08u) == 0u) {
        return;
    }

    g_packet[g_packet_index++] = data;
    if (g_packet_index < 3u) {
        return;
    }
    g_packet_index = 0u;

    const uint8_t buttons = (uint8_t)(g_packet[0] & 0x07u);
    const uint8_t changed = (uint8_t)(buttons ^ g_last_buttons);
    g_last_buttons = buttons;

    const int8_t dx = (int8_t)g_packet[1];
    const int8_t dy = (int8_t)(-(int8_t)g_packet[2]);

    mouse_event_t event;
    event.dx = dx;
    event.dy = dy;
    event.buttons = buttons;
    event.changed = changed;
    mouse_queue_push(event);
}
