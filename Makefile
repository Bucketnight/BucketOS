CC := gcc
AS := gcc
LD := gcc
OBJCOPY := objcopy
HOST_LD := ld
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
INITRD_ROOT := $(BUILD_DIR)/initrd
KERNEL := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/BucketKernel.iso
INITRD := $(BUILD_DIR)/initrd.tar
USERTEST_ELF := $(BUILD_DIR)/usertest.elf
USERTEST_BIN := $(BUILD_DIR)/usertest.bin
GENERATED_CONFIG := include/bucketos/config.h
WALLPAPER_SRC := logo.png
WALLPAPER_RAW := $(INITRD_ROOT)/usr/share/wallpaper.bgra

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
	src/kernel/mouse.c \
	src/kernel/hypervisor.c \
	src/kernel/serial.c \
	src/kernel/console.c \
	src/kernel/framebuffer.c \
	src/kernel/paging.c \
	src/kernel/exec.c \
	src/kernel/syscall.c \
	src/kernel/usertest.c \
	src/kernel/process.c \
	src/kernel/scheduler.c \
	src/kernel/ramfs.c \
	src/kernel/devfs.c \
	src/kernel/vfs.c \
	src/kernel/memory.c \
	src/kernel/panic.c \
	src/kernel/shell.c \


SOURCES_S := \
	src/arch/x86/boot.s \
	src/arch/x86/isr_stubs.s

USER_SOURCES_S := \
	src/user/crt0.s \
	src/user/usertest.s

USER_SOURCES_C := \
	src/user/lib.c \
	src/user/fbtest.c \
	src/user/sh.c \
	src/user/wm.c

OBJECTS := \
	$(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SOURCES_C)) \
	$(patsubst src/%.s,$(OBJ_DIR)/%.o,$(SOURCES_S))

USER_OBJECTS := \
	$(patsubst src/%.s,$(OBJ_DIR)/%.o,$(USER_SOURCES_S)) \
	$(patsubst src/%.c,$(OBJ_DIR)/%.o,$(USER_SOURCES_C))

USER_CFLAGS := -m32 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra -Werror -O2 -Iinclude -Iuser/include

USERCRT_OBJECT := $(OBJ_DIR)/user/crt0.o
USERLIB_OBJECT := $(OBJ_DIR)/user/lib.o
USERTEST_OBJECT := $(OBJ_DIR)/user/usertest.o
USERFBTEST_OBJECT := $(OBJ_DIR)/user/fbtest.o
USERSH_OBJECT := $(OBJ_DIR)/user/sh.o
USERWM_OBJECT := $(OBJ_DIR)/user/wm.o

USERSH_ELF := $(BUILD_DIR)/sh.elf
USERFBTEST_ELF := $(BUILD_DIR)/fbtest.elf
USERWM_ELF := $(BUILD_DIR)/wm.elf

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

$(OBJ_DIR)/user/%.o: src/user/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)printf "UC     %s\n" "$<"
	$(Q)$(CC) $(USER_CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.c $(GENERATED_CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)printf "C      %s\n" "$<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.s
	$(Q)mkdir -p $(dir $@)
	$(Q)printf "AS     %s\n" "$<"
	$(Q)$(AS) $(ASFLAGS) -c $< -o $@

$(USERTEST_ELF): $(USER_OBJECTS) user.ld | $(BUILD_DIR)
	$(Q)printf "LD     %s\n" "$@"
	$(Q)$(HOST_LD) -m elf_i386 -T user.ld -nostdlib -o $@ $(USERTEST_OBJECT)

$(USERSH_ELF): $(USERCRT_OBJECT) $(USERLIB_OBJECT) $(USERSH_OBJECT) user.ld | $(BUILD_DIR)
	$(Q)printf "LD     %s\n" "$@"
	$(Q)$(HOST_LD) -m elf_i386 -T user.ld -nostdlib -o $@ $(USERCRT_OBJECT) $(USERLIB_OBJECT) $(USERSH_OBJECT)

$(USERFBTEST_ELF): $(USERCRT_OBJECT) $(USERLIB_OBJECT) $(USERFBTEST_OBJECT) user.ld | $(BUILD_DIR)
	$(Q)printf "LD     %s\n" "$@"
	$(Q)$(HOST_LD) -m elf_i386 -T user.ld -nostdlib -o $@ $(USERCRT_OBJECT) $(USERLIB_OBJECT) $(USERFBTEST_OBJECT)

$(USERWM_ELF): $(USERCRT_OBJECT) $(USERLIB_OBJECT) $(USERWM_OBJECT) user.ld | $(BUILD_DIR)
	$(Q)printf "LD     %s\n" "$@"
	$(Q)$(HOST_LD) -m elf_i386 -T user.ld -nostdlib -o $@ $(USERCRT_OBJECT) $(USERLIB_OBJECT) $(USERWM_OBJECT)

$(USERTEST_BIN): $(USERTEST_ELF)
	$(Q)printf "BIN    %s\n" "$@"
	$(Q)$(OBJCOPY) -O binary $< $@

$(BUILD_DIR):
	$(Q)mkdir -p $(BUILD_DIR)

$(ISO_DIR):
	$(Q)mkdir -p $(ISO_DIR)/boot/grub

$(INITRD_ROOT):
	$(Q)mkdir -p $(INITRD_ROOT)

$(GENERATED_CONFIG): $(CONFIG_FILE) scripts/genconfig.sh
	$(Q)printf "CONF   %s\n" "$@"
	$(Q)$(GENCONFIG) $(CONFIG_FILE) $@

$(INITRD): $(USERTEST_BIN) $(USERSH_ELF) $(USERFBTEST_ELF) $(USERWM_ELF) | $(BUILD_DIR) $(INITRD_ROOT)
	$(Q)printf "TAR    %s\n" "$@"
	$(Q)rm -rf $(INITRD_ROOT)
	$(Q)mkdir -p $(INITRD_ROOT)
	$(Q)cp -R initrd/. $(INITRD_ROOT)/
	$(Q)mkdir -p $(INITRD_ROOT)/bin
	$(Q)mkdir -p $(INITRD_ROOT)/usr/share
	$(Q)cp $(USERTEST_ELF) $(INITRD_ROOT)/bin/usertest.elf
	$(Q)cp $(USERSH_ELF) $(INITRD_ROOT)/bin/sh.elf
	$(Q)cp $(USERFBTEST_ELF) $(INITRD_ROOT)/bin/fbtest.elf
	$(Q)cp $(USERWM_ELF) $(INITRD_ROOT)/bin/wm.elf
	$(Q)printf "WALL   %s\n" "$(WALLPAPER_RAW)"
	$(Q)magick $(WALLPAPER_SRC) -alpha on -depth 8 BGRA:$(WALLPAPER_RAW)
	$(Q)tar --format=ustar -cf $@ -C $(INITRD_ROOT) .
