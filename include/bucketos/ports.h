#ifndef BUCKETKERNEL_PORTS_H
#define BUCKETKERNEL_PORTS_H

#include "bucketkernel/common.h"

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);
void io_wait(void);
uint16_t read_cs(void);
void cpu_halt(void);
void outw(uint16_t port, uint16_t value);


#endif
