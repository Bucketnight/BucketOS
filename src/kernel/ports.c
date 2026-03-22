#include "bucketos/ports.h"

uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

uint16_t read_cs(void) {
    uint16_t cs;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    return cs;
}

void cpu_halt(void) {
    __asm__ volatile ("hlt");
}

void cpu_reboot(void) {
    __asm__ volatile ("cli");

    while ((inb(0x64) & 0x02u) != 0u) {
    }

    outb(0x64, 0xFE);

    for (;;) {
        cpu_halt();
    }
}

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}
