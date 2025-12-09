ThorOS is a **minimal, lightweight educational operating system kernel** written in C, designed for learning OS development, VGA output, keyboard handling, and basic shell functionalities.

---

## Features

- Custom VGA terminal driver with colored output
- Interactive shell with:
  - Built-in commands: `help`, `cls`, `echo`, `ls`, `ls -l`, `time`, `mem`, `cpu`, `rand`, `whoami`, `ver`, `halt`, `reboot`
  - Command history (Up arrow)
  - Tab auto-completion
  - Ctrl+C to clear input
- Mock file system for demonstration (`ls` commands)
- Basic kernel utilities:
  - Integer to string conversion
  - Keyboard input handling
- Fully bootable using GRUB or QEMU

---

## Prerequisites

To build and run ThorOS, you need:

- `gcc` / `i686-elf-gcc` cross-compiler  
- `make`  
- `nasm` (for bootloader if used)  
- `qemu` (for testing in a virtual machine)  

Optional for real hardware testing:

- USB drive or bootable ISO creation tools

---

## Build Instructions

1. Clone the repository:
git clone https://github.com/yourusername/ThorOs.git
cd src

2. Compile the kernel:
   make
   This will generate kernel.elf in the build directory

3. Create a bootable ISO (if using GRUB):
   mkdir -p iso/boot/grub
   cp build/kernel.elf iso/boot/
   cp grub.cfg iso/boot/grub/
   grub-mkrescue -o nebulaos.iso iso/

Running KernelPanicOS in QEMU

qemu-system-i386 -cdrom kernel.iso -m 512M

Available Commands

| Command       | Description                  |
| ------------- | ---------------------------- |
| `help`        | Show all commands            |
| `cls`         | Clear the screen             |
| `echo <text>` | Print text                   |
| `ls`          | List files (mock filesystem) |
| `ls -l`       | List files with details      |
| `time`        | Show kernel uptime           |
| `mem`         | Show memory info             |
| `cpu`         | Show CPU information         |
| `rand`        | Print a random number        |
| `whoami`      | Show current user (`root`)   |
| `ver`         | Show kernel version          |
| `reboot`      | Restart the OS               |
| `halt`        | Stop the CPU                 |
| Ctrl+C        | Cancel current input         |



    
