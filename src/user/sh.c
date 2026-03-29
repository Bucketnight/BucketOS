// sh.c: Userspace shell (parses commands and execs /bin/<program> programs).

/*
 * Reading guide:
 * - Purpose: sh.c: Userspace shell (parses commands and execs /bin/<program> programs).
 * - Start reading at: main
 * - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).
 */

#include "user/lib.h"

enum {
    SHELL_BUFFER_SIZE = 128,
    SHELL_LIST_BUFFER_SIZE = 512,
    SHELL_READ_BUFFER_SIZE = 128
};

static void shell_write_line(const char *text) {
    write_str(text);
    write_str("\n");
}

static int shell_read_line(char *buffer, int size) {
    int length = 0;

    for (;;) {
        char c = '\0';
        const int result = sys_read(0, &c, 1);

        if (result < 0) {
            return -1;
        }
        if (result == 0) {
            sys_yield();
            continue;
        }

        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            buffer[length] = '\0';
            return length;
        }
        if (c == '\b') {
            if (length > 0) {
                --length;
            }
            continue;
        }
        if (length + 1 < size) {
            buffer[length++] = c;
        }
    }
}

static void shell_cat(const char *path) {
    char buffer[SHELL_READ_BUFFER_SIZE];
    const int fd = sys_open(path);

    if (fd < 0) {
        shell_write_line("cat: file not found");
        return;
    }

    for (;;) {
        const int count = sys_read(fd, buffer, sizeof(buffer));
        if (count < 0) {
            shell_write_line("cat: read failed");
            break;
        }
        if (count == 0) {
            break;
        }
        sys_write(1, buffer, count);
    }

    sys_close(fd);
    write_str("\n");
}

static void shell_ls(const char *path) {
    char buffer[SHELL_LIST_BUFFER_SIZE];
    const int count = sys_list(path, buffer, sizeof(buffer));

    if (count < 0) {
        shell_write_line("ls: path not found");
        return;
    }

    sys_write(1, buffer, count);
}

static void shell_execute(char *line) {
    char *args = line;

    while (*args != '\0' && *args != ' ') {
        ++args;
    }

    if (*args != '\0') {
        *args++ = '\0';
        args = skip_spaces(args);
    }

    if (strcmp(line, "help") == 0) {
        shell_write_line("commands: help echo cat ls readme clear exit");
    } else if (strcmp(line, "echo") == 0) {
        shell_write_line(args);
    } else if (strcmp(line, "cat") == 0) {
        if (*args == '\0') {
            shell_write_line("cat: missing path");
        } else {
            shell_cat(args);
        }
    } else if (strcmp(line, "ls") == 0) {
        shell_ls(*args == '\0' ? "/" : args);
    } else if (strcmp(line, "readme") == 0) {
        shell_cat("/readme.txt");
    } else if (strcmp(line, "clear") == 0) {
        sys_clear();
    } else if (strcmp(line, "exit") == 0) {
        sys_exit(0);
    } else if (*line != '\0') {
        shell_write_line("unknown command");
    }
}

int main(int argc, char **argv) {
    char line[SHELL_BUFFER_SIZE];

    (void)argc;
    (void)argv;

    shell_write_line("BucketOS user shell");

    for (;;) {
        write_str("ush> ");
        if (shell_read_line(line, sizeof(line)) < 0) {
            shell_write_line("shell: read failed");
            return 1;
        }
        shell_execute(line);
    }
}
