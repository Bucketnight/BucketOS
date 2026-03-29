# crt0.s: Userspace CRT0 (entry point that calls main(argc, argv) then exits).

# Reading guide:
# - Purpose: crt0.s: Userspace CRT0 (entry point that calls main(argc, argv) then exits).
# - Start reading at: _start
# - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).

.section .text
.global _start
.extern main

_start:
    mov (%esp), %eax
    lea 4(%esp), %edx
    push %edx
    push %eax
    call main
    mov %eax, %ebx
    mov $2, %eax
    int $0x80

1:
    jmp 1b
