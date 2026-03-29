    // lib.c: Userspace runtime (syscall wrappers + tiny string helpers).

/*
 * Reading guide:
 * - Purpose: lib.c: Userspace runtime (syscall wrappers + tiny string helpers).
 * - Start reading at: user_syscall0
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#include "user/lib.h"
#include "user/syscall.h"

int user_syscall0(int number) {
    int result;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number) : "memory");
    return result;
}

int user_syscall1(int number, int arg0) {
    int result;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "b"(arg0) : "memory");
    return result;
}

int user_syscall2(int number, int arg0, int arg1) {
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(number), "b"(arg0), "c"(arg1)
        : "memory");
    return result;
}

int user_syscall3(int number, int arg0, int arg1, int arg2) {
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(number), "b"(arg0), "c"(arg1), "d"(arg2)
        : "memory");
    return result;
}

int user_syscall4(int number, int arg0, int arg1, int arg2, int arg3) {
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(number), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3)
        : "memory");
    return result;
}

int sys_open(const char *path) {
    return user_syscall1(USER_SYSCALL_OPEN, (int)path);
}

int sys_read(int fd, void *buffer, int size) {
    return user_syscall3(USER_SYSCALL_READ, fd, (int)buffer, size);
}

int sys_write(int fd, const void *buffer, int size) {
    return user_syscall3(USER_SYSCALL_WRITE, fd, (int)buffer, size);
}

int sys_close(int fd) {
    return user_syscall1(USER_SYSCALL_CLOSE, fd);
}

int sys_yield(void) {
    return user_syscall0(USER_SYSCALL_YIELD);
}

int sys_list(const char *path, char *buffer, int size) {
    return user_syscall3(USER_SYSCALL_LIST, (int)path, (int)buffer, size);
}

int sys_clear(void) {
    return user_syscall0(USER_SYSCALL_CLEAR);
}

void sys_exit(int code) {
    user_syscall1(USER_SYSCALL_EXIT, code);
    for (;;) {
    }
}

void write_str(const char *text) {
    sys_write(1, text, strlen(text));
}

int strlen(const char *text) {
    int length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

int strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

int strncmp(const char *left, const char *right, int count) {
    for (int index = 0; index < count; ++index) {
        if (left[index] != right[index] || left[index] == '\0' || right[index] == '\0') {
            return (unsigned char)left[index] - (unsigned char)right[index];
        }
    }
    return 0;
}

char *skip_spaces(char *text) {
    while (*text == ' ') {
        ++text;
    }
    return text;
}
