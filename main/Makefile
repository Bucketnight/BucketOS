# Makefile for Bucket OS (C Kernel)

# Compiler and tools
CC = gcc
AS = nasm
LD = ld

# Flags - COMPLETE bare metal flags
CFLAGS = -m32 -ffreestanding -fno-pie -fno-pic -static \
         -fno-stack-protector -nostdlib \
         -mno-red-zone -fno-exceptions -fno-asynchronous-unwind-tables \
         -mgeneral-regs-only -mno-sse -mno-mmx -mno-80387 \
         -fno-builtin -fno-common \
         -Wall -Wextra -O1 -fno-omit-frame-pointer -I.

LDFLAGS = -m elf_i386 -T linker.ld --oformat binary -nostdlib

ASFLAGS = -f bin

# Files
KERNEL_C = kernel.c
KERNEL_O = kernel.o
START_ASM = start.asm
START_O = start.o
KERNEL_BIN = kernel.bin
BOOT_ASM = boot.asm
BOOT_BIN = boot.bin
OS_IMG = os.img

# Build targets
.PHONY: all clean run

all: $(OS_IMG)

# Compile C kernel 
$(KERNEL_O): $(KERNEL_C)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble start.asm (entry point)
$(START_O): $(START_ASM)
	nasm -f elf32 $< -o $@

# Link kernel (start.o must come FIRST)
$(KERNEL_BIN): $(START_O) $(KERNEL_O) linker.ld
	$(LD) $(LDFLAGS) $(START_O) $(KERNEL_O) -o $@

# Assemble bootloader
$(BOOT_BIN): $(BOOT_ASM)
	$(AS) $(ASFLAGS) $< -o $@

# Create disk image
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "✓ Built Bucket OS: $@"
	@echo "  Boot sector: $$(stat -c%s $(BOOT_BIN)) bytes"
	@echo "  Kernel: $$(stat -c%s $(KERNEL_BIN)) bytes"

# Run in QEMU
run: $(OS_IMG)
	qemu-system-i386 -drive format=raw,file=$(OS_IMG)

# Run with debugging
debug: $(OS_IMG)
	qemu-system-i386 -drive format=raw,file=$(OS_IMG) -d int,cpu_reset -no-reboot

# Clean build artifacts
clean:
	rm -f $(KERNEL_O) $(START_O) $(KERNEL_BIN) $(BOOT_BIN) $(OS_IMG)
	@echo "✓ Cleaned build artifacts"