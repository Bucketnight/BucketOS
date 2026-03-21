#ifndef BUCKETKERNEL_STRING_H
#define BUCKETKERNEL_STRING_H

#include "bucketkernel/common.h"

void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *dest, int value, size_t count);
size_t strlen(const char *str);
int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t count);

#endif
