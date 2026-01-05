# 🚀 Bucket OS - Full Featured C Kernel

A complete operating system written in C with a graphical user interface, mouse support, keyboard input, and window management.

## ✨ Features

### Core Features
- ✅ **Protected Mode** - 32-bit protected mode with GDT
- ✅ **VGA Graphics** - Mode 13h (320x200, 256 colors)
- ✅ **Double Buffering** - Flicker-free rendering
- ✅ **PS/2 Mouse Driver** - Full 3-byte packet handling
- ✅ **Keyboard Driver** - Scancode reading with ring buffer
- ✅ **PIC Management** - Proper IRQ remapping

### GUI Features
- ✅ **Desktop** - Gradient background
- ✅ **Taskbar** - With START button
- ✅ **Start Menu** - Pop-up menu system
- ✅ **Desktop Icons** - Computer, Files, Notes
- ✅ **Windows** - Draggable windows with title bars
- ✅ **Mouse Cursor** - Custom cursor with shadow
- ✅ **3D Buttons** - Highlight and shadow effects

### Window Management
- ✅ **Create Windows** - Dynamic window creation
- ✅ **Drag Windows** - Click and drag title bars
- ✅ **Window Shadows** - Drop shadows for depth
- ✅ **Close Buttons** - Functional close buttons
- ✅ **Z-Order** - Proper window layering

## 🛠️ Building

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get install gcc nasm qemu-system-x86 make binutils
```

**Arch Linux:**
```bash
sudo pacman -S gcc nasm qemu-system-x86 make binutils
```

**macOS (with Homebrew):**
```bash
brew install i686-elf-gcc nasm qemu make
# Note: Use i686-elf-gcc instead of gcc in Makefile
```

### Build Commands

**Build everything:**
```bash
make
```

**Build and run in QEMU:**
```bash
make run
```

**Build and run with debugging:**
```bash
make debug
```

**Clean build artifacts:**
```bash
make clean
```

### Manual Build

If you prefer to build manually:

```bash
# 1. Assemble bootloader
nasm -f bin boot.asm -o boot.bin

# 2. Compile C kernel
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib \
    -mno-red-zone -fno-exceptions -fno-asynchronous-unwind-tables \
    -Wall -Wextra -O2 -c kernel.c -o kernel.o

# 3. Link kernel
ld -m elf_i386 -T linker.ld kernel.o -o kernel.bin

# 4. Create disk image
dd if=/dev/zero of=os.img bs=512 count=2880
dd if=boot.bin of=os.img bs=512 count=1 conv=notrunc
dd if=kernel.bin of=os.img bs=512 seek=1 conv=notrunc

# 5. Run
qemu-system-i386 -drive format=raw,file=os.img
```

## 🎮 Controls

### Keyboard
- **Arrow Keys** - Move mouse cursor (for testing without mouse)
- **Enter** - Simulate mouse click
- **ESC** - Toggle start menu

### Mouse
- **Left Click** - Click buttons, select icons, drag windows
- **Drag** - Click and drag window title bars to move windows

## 📂 Project Structure

```
bucket-os/
├── kernel.c         # Main C kernel with all features
├── boot.asm         # Bootloader (real mode → protected mode)
├── linker.ld        # Linker script for kernel
├── Makefile         # Build system
├── README.md        # This file
└── os.img          # Output disk image (generated)
```

## 🏗️ Architecture

### Boot Process
1. **BIOS** loads bootloader (boot.asm) at 0x7C00
2. **Bootloader** sets VGA mode 13h
3. **Bootloader** loads C kernel from disk to 0x10000
4. **Bootloader** enables A20 line
5. **Bootloader** loads GDT and enters protected mode
6. **Bootloader** copies kernel to 0x100000 (1MB)
7. **Bootloader** jumps to C kernel entry point
8. **Kernel** initializes hardware and GUI
9. **Kernel** enters main event loop

### Memory Layout
- `0x00000000 - 0x000003FF`: Real Mode IVT
- `0x00000400 - 0x000004FF`: BIOS Data Area
- `0x00000500 - 0x00007BFF`: Free (usable)
- `0x00007C00 - 0x00007DFF`: Bootloader
- `0x00010000 - 0x0001FFFF`: Kernel (temporary)
- `0x00090000 - 0x0009FFFF`: Stack
- `0x000A0000 - 0x000AFFFF`: VGA Memory
- `0x00100000+`: Kernel (final location at 1MB)

### Code Organization

**kernel.c** contains:
- Hardware drivers (PIC, PS/2 Mouse, Keyboard)
- Graphics primitives (pixels, rectangles, text)
- UI rendering (desktop, taskbar, windows)
- Window management (create, drag, close)
- Event handling (mouse clicks, keyboard input)
- Main loop

## 🐛 Debugging

### Common Issues

**"Disk read error!"**
- Boot sector couldn't load kernel
- Check that kernel.bin exists
- Verify disk image creation commands

**Black screen after boot**
- VGA mode not set correctly
- Check that boot.asm sets mode 13h
- Verify BIOS interrupt 0x10 works

**Kernel crashes/reboots**
- Triple fault in protected mode
- Check GDT configuration
- Verify memory addresses
- Use `make debug` to see fault info

### Debugging with QEMU

```bash
# Show CPU state on crashes
qemu-system-i386 -drive format=raw,file=os.img -d int,cpu_reset -no-reboot

# Save debug log
qemu-system-i386 -drive format=raw,file=os.img -d int -D qemu.log

# Monitor mode
qemu-system-i386 -drive format=raw,file=os.img -monitor stdio
```

## 🚧 Future Enhancements

Potential additions:
- [ ] Real PS/2 mouse interrupts (IRQ12)
- [ ] Keyboard interrupts (IRQ1)
- [ ] Timer interrupt (IRQ0) for proper scheduling
- [ ] Memory manager with heap allocation
- [ ] File system (FAT12/FAT16)
- [ ] More window types (text editor, calculator)
- [ ] Sound support (PC speaker/Sound Blaster)
- [ ] Network stack (basic TCP/IP)
- [ ] Multi-tasking/processes

## 📝 Notes

- This is an educational OS demonstrating OS development concepts
- No real hardware protection or security features
- Mouse movement is relative, not absolute
- No disk I/O after boot (runs entirely from loaded image)
- Font is hardcoded bitmap (8x8 pixels)
- Colors are VGA 256-color palette

## 📄 License

This is educational code. Feel free to learn from it, modify it, and use it for educational purposes.

## 🙏 Acknowledgments

Built with:
- GCC - GNU Compiler Collection
- NASM - Netwide Assembler
- QEMU - Quick EMUlator
- OSDev Wiki - Invaluable resource

---

**Happy OS Development! 🎉**
