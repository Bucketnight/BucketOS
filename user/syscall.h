// syscall.h: Userspace syscall numbers and inline syscall helpers.

/*
 * Reading guide:
 * - Purpose: syscall.h: Userspace syscall numbers and inline syscall helpers.
 * - Start reading at: user_syscall0
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

enum {
    USER_SYSCALL_WRITE = 1,
    USER_SYSCALL_EXIT = 2,
    USER_SYSCALL_OPEN = 3,
    USER_SYSCALL_READ = 4,
    USER_SYSCALL_CLOSE = 5,
    USER_SYSCALL_YIELD = 6,
    USER_SYSCALL_LIST = 7,
    USER_SYSCALL_CLEAR = 8,
    USER_SYSCALL_FB_INFO = 9,
    USER_SYSCALL_FB_PUTPIXEL = 10,
    USER_SYSCALL_FB_FILL_RECT = 11,
    USER_SYSCALL_FB_BLIT = 12
};

int user_syscall0(int number);
int user_syscall1(int number, int arg0);
int user_syscall2(int number, int arg0, int arg1);
int user_syscall3(int number, int arg0, int arg1, int arg2);
int user_syscall4(int number, int arg0, int arg1, int arg2, int arg3);

#endif
