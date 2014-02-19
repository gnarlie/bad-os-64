CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector
DISPLAY_LIBRARY ?= sdl

default: kernel.sys

run: disk.img
	IMAGE=disk.img DISPLAY_LIBRARY=$(DISPLAY_LIBRARY) CYL=128 LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libXpm.so.4 bochs -f etc/bochsrc

out/%.o: src/%.c
	$(CC) -c $(CFLAGS) -o $@ $^

out/%.o: src/%.asm
	@mkdir -p out/
	nasm $< -felf64 -o $@

kernel.sys: out/entry.o out/main.o out/console.o out/keyboard.o
	ld -Tsrc/kernel.ld -melf_x86_64 -o /tmp/kernel $^
	cat bootloader/pure64.sys /tmp/kernel > kernel.sys

disk.img: kernel.sys
	./createimage.sh disk.img 1 bootloader/bmfs_mbr.sys kernel.sys

clean:
	rm -f disk.img entry.sys kernel.sys out/*

.PHONY: run clean
