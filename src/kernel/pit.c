#include "bucketkernel/pit.h"
#include "bucketkernel/ports.h"

static uint32_t g_ticks;

void pit_initialize(uint32_t frequency_hz) {
    const uint32_t divisor = 1193180u / frequency_hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFFu));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFFu));
}

void pit_handle_tick(void) {
    ++g_ticks;
}

uint32_t pit_ticks(void) {
    return g_ticks;
}
