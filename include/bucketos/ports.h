#ifndef BUCKETOS_PORTS_H
#define BUCKETOS_PORTS_H

#include "bucketos/common.h"

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);
void io_wait(void);
uint16_t read_cs(void);
void cpu_halt(void);
void outw(uint16_t port, uint16_t value);
void cpu_reboot(void) __attribute__((noreturn));


#endif
