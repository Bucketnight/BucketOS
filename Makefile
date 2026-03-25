CC := gcc
AS := gcc
LD := gcc
OBJCOPY := objcopy
CONFIG_FILE ?= .config
GENCONFIG := ./scripts/genconfig.sh
GRUB_MKRESCUE ?= grub2-mkrescue
QEMU ?= qemu-system-x86_64
Q := @

CFLAGS := -m32 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra -Werror -O2 -Iinclude
ASFLAGS := -m32 -ffreestanding
LDFLAGS := -m32 -T linker.ld -nostdlib -no-pie

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
ISO_DIR := $(BUILD_DIR)/iso
KERNEL := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/BucketKernel.iso
INITRD := $(BUILD_DIR)/initrd.tar
GENERATED_CONFIG := include/bucketos/config.h

SOURCES_C := \
	src/kernel/kernel.c \
	src/kernel/terminal.c \
	src/kernel/string.c \
	src/kernel/print.c \
	src/kernel/ports.c \
	src/kernel/gdt.c \
	src/kernel/idt.c \
	src/kernel/interrupts.c \
	src/kernel/pit.c \
	src/kernel/keyboard.c \
	src/kernel/hypervisor.c \
	src/kernel/serial.c \
	src/kernel/framebuffer.c \
	src/kernel/paging.c \
	src/kernel/syscall.c \
	src/kernel/usertest.c \
	src/kernel/ramfs.c \
	src/kernel/devfs.c \
	src/kernel/vfs.c \
	src/kernel/memory.c \
	src/kernel/panic.c \
	src/kernel/shell.c \


SOURCES_S := \
	src/arch/x86/boot.s \
	src/arch/x86/isr_stubs.s

OBJECTS := \
	$(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SOURCES_C)) \
	$(patsubst src/%.s,$(OBJ_DIR)/%.o,$(SOURCES_S))

.PHONY: all kernel iso run run-serial clean defconfig config

all: kernel

kernel: $(KERNEL)

iso: $(ISO)

run: $(ISO)
	$(Q)printf "RUN    %s\n" "$(ISO)"
	$(Q)$(QEMU) -cdrom $(ISO)

run-serial: $(ISO)
	$(Q)printf "RUN    %s\n" "$(ISO)"
	$(Q)$(QEMU) -cdrom $(ISO) -display none -serial stdio

clean:
	$(Q)printf "CLEAN  %s\n" "$(BUILD_DIR)"
	$(Q)rm -rf $(BUILD_DIR)
	$(Q)rm -f $(GENERATED_CONFIG)

defconfig: configs/defconfig
	$(Q)printf "CONF   %s\n" "$(CONFIG_FILE)"
	$(Q)cp $< $(CONFIG_FILE)

config: $(GENERATED_CONFIG)

$(KERNEL): $(OBJECTS) linker.ld | $(BUILD_DIR)
	$(Q)printf "LD     %s\n" "$@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJECTS) -lgcc

$(ISO): $(KERNEL) $(INITRD) iso/boot/grub/grub.cfg | $(ISO_DIR)
	$(Q)printf "ISO    %s\n" "$@"
	$(Q)cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	$(Q)cp $(INITRD) $(ISO_DIR)/boot/initrd.tar
	$(Q)cp iso/boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(Q)$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

$(OBJ_DIR)/%.o: src/%.c $(GENERATED_CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)printf "C      %s\n" "$<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.s
	$(Q)mkdir -p $(dir $@)
	$(Q)printf "AS     %s\n" "$<"
	$(Q)$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR):
	$(Q)mkdir -p $(BUILD_DIR)

$(ISO_DIR):
	$(Q)mkdir -p $(ISO_DIR)/boot/grub

$(GENERATED_CONFIG): $(CONFIG_FILE) scripts/genconfig.sh
	$(Q)printf "CONF   %s\n" "$@"
	$(Q)$(GENCONFIG) $(CONFIG_FILE) $@

$(INITRD): | $(BUILD_DIR)
	$(Q)printf "TAR    %s\n" "$@"
	$(Q)tar --format=ustar -cf $@ -C initrd .
