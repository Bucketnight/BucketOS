// string.c: Implementations of memset/memcpy/strcmp/strlen/etc.

/*
 * Reading guide:
 * - Purpose: string.c: Implementations of memset/memcpy/strcmp/strlen/etc.
 * - Start reading at: strlen
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/string.h"

void *memcpy(void *dest, const void *src, size_t count) {
    unsigned char *out = (unsigned char *)dest;
    const unsigned char *in = (const unsigned char *)src;

    for (size_t i = 0; i < count; ++i) {
        out[i] = in[i];
    }

    return dest;
}

void *memset(void *dest, int value, size_t count) {
    unsigned char *out = (unsigned char *)dest;

    for (size_t i = 0; i < count; ++i) {
        out[i] = (unsigned char)value;
    }

    return dest;
}

size_t strlen(const char *str) {
    size_t length = 0;

    while (str[length] != '\0') {
        ++length;
    }

    return length;
}

int strcmp(const char *lhs, const char *rhs) {
    while (*lhs != '\0' && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }

    return (unsigned char)*lhs - (unsigned char)*rhs;
}

int strncmp(const char *lhs, const char *rhs, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (lhs[i] != rhs[i] || lhs[i] == '\0' || rhs[i] == '\0') {
            return (unsigned char)lhs[i] - (unsigned char)rhs[i];
        }
    }

    return 0;
}
