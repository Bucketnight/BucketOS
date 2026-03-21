#ifndef BUCKETKERNEL_PRINT_H
#define BUCKETKERNEL_PRINT_H

#include "bucketkernel/common.h"

void print_char(char c);
void print_string(const char *text);
void print_line(const char *text);
void print_uint32(uint32_t value);
void print_hex32(uint32_t value);
void print_banner(void);
void print_hex64(uint64_t value);

#endif
