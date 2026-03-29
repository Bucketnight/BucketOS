# usertest.s: Tiny userspace test program written in assembly.

# Reading guide:
# - Purpose: usertest.s: Tiny userspace test program written in assembly.
# - Start reading at: _start
# - Tip: In userspace, everything goes through syscalls and file descriptors (/dev/<device> and /bin/<program>).

.section .text
.global _start

_start:
    mov $3, %eax
    mov $path, %ebx
    int $0x80
    mov %eax, %esi

    mov $4, %eax
    mov %esi, %ebx
    mov $buffer, %ecx
    mov $(buffer_end - buffer), %edx
    int $0x80
    mov %eax, %edi

    mov $1, %eax
    mov $1, %ebx
    mov $buffer, %ecx
    mov %edi, %edx
    int $0x80

    mov $5, %eax
    mov %esi, %ebx
    int $0x80

    mov $2, %eax
    xor %ebx, %ebx
    int $0x80

path:
    .asciz "/readme.txt"

buffer:
    .space 128
buffer_end:
