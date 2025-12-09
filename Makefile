CC = gcc
LD = ld

CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie
LDFLAGS = -T src/kernel.ld -m elf_i386 -nostdlib

all: iso

kernel.elf: boot.o kernel.o
	$(LD) $(LDFLAGS) boot.o kernel.o -o kernel.elf

boot.o: src/boot.S
	$(CC) -c src/boot.S -o boot.o $(CFLAGS)

kernel.o: src/kernel.c
	$(CC) -c src/kernel.c -o kernel.o $(CFLAGS)

iso: kernel.elf
	mkdir -p iso/boot/grub
	cp kernel.elf iso/boot/kernel.elf
	cp grub/grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o mykernel.iso iso

run: iso
	qemu-system-i386 -cdrom mykernel.iso

clean:
	rm -rf *.o iso kernel.elf mykernel.iso
