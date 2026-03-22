#ifndef BUCKETOS_SERIAL_H
#define BUCKETOS_SERIAL_H

#include "bucketos/common.h"

void serial_initialize(void);
bool serial_is_ready(void);
void serial_write_char(char c);
void serial_write(const char *text);
void serial_write_line(const char *text);

#endif
