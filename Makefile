CC := gcc
AS := gcc
LD := gcc
OBJCOPY := objcopy
GRUB_MKRESCUE ?= grub2-mkrescue
QEMU ?= qemu-system-x86_64

CFLAGS := -m32 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra -Werror -O2 -Iinclude
ASFLAGS := -m32 -ffreestanding
LDFLAGS := -m32 -T linker.ld -nostdlib -no-pie

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
ISO_DIR := $(BUILD_DIR)/iso
KERNEL := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/BucketKernel.iso

SOURCES_C := \
	src/kernel/kernel.c \
	src/kernel/terminal.c \
	src/kernel/string.c \
	src/kernel/print.c \
	src/kernel/ports.c \
	src/kernel/idt.c \
	src/kernel/interrupts.c \
	src/kernel/pit.c \
	src/kernel/keyboard.c \
	src/kernel/hypervisor.c \
	src/kernel/memory.c \
	src/kernel/shell.c \


SOURCES_S := \
	src/arch/x86/boot.s \
	src/arch/x86/isr_stubs.s

OBJECTS := \
	$(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SOURCES_C)) \
	$(patsubst src/%.s,$(OBJ_DIR)/%.o,$(SOURCES_S))

.PHONY: all kernel iso run clean

all: kernel

kernel: $(KERNEL)

iso: $(ISO)

run: $(ISO)
	$(QEMU) -cdrom $(ISO)

clean:
	rm -rf $(BUILD_DIR)

$(KERNEL): $(OBJECTS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) -lgcc

$(ISO): $(KERNEL) iso/boot/grub/grub.cfg | $(ISO_DIR)
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	cp iso/boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

$(OBJ_DIR)/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.s
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ISO_DIR):
	mkdir -p $(ISO_DIR)/boot/grub
