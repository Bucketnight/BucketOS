// lib.h: Userspace runtime header (syscall wrappers + small helpers).

/*
 * Reading guide:
 * - Purpose: lib.h: Userspace runtime header (syscall wrappers + small helpers).
 * - Start reading at: sys_open
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#ifndef USER_LIB_H
#define USER_LIB_H

int sys_open(const char *path);
int sys_read(int fd, void *buffer, int size);
int sys_write(int fd, const void *buffer, int size);
int sys_close(int fd);
int sys_yield(void);
int sys_list(const char *path, char *buffer, int size);
int sys_clear(void);
void sys_exit(int code);

void write_str(const char *text);
int strlen(const char *text);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, int count);
char *skip_spaces(char *text);

#endif
