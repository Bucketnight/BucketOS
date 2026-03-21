# BucketOS
This is a solo project (or small project) made by me (Bucketnight)

# THIS IS STILL IN DEVELOPMENT. IF YOU WANT TO CONTRIBUTE, FEEL FREE

---

# How to build and use
## Tools:
- Linux (Or WSL for Windows users)
- `gcc`
- `nasm`
- `make`
- `qemu` (This is needed)
- `git`
- `grub`
- `make`
- `xorriso`
- `binutils`

---
## Install tools:
### Arch:
- `sudo pacman -S gcc nasm make qemu-desktop git grub make xorriso binutils`
### Ubuntu, Debian-based:
- `sudo apt install build-essential nasm qemu-system-x86 git grub-pc-bin xorriso binutils grub-common`
### Fedora:
- `sudo dnf install gcc nasm make qemu-system-x86 git grub2-tools xorriso binutils`

---
## Build:
`make`

---
## Run:
`make run`

--
## Clean:
`make clean`
