CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -std=gnu99 -Werror -MMD -MP
DISPLAY_LIBRARY ?= sdl

C_FILES=$(shell find src -name '*.c')
ASM_FILES=$(shell find src -name '*.asm')

OBJS=$(patsubst src/%.c, out/%.o, $(C_FILES)) $(patsubst src/%.asm, out/%.o, $(ASM_FILES))

default: kernel.sys

run: disk.img
	IMAGE=disk.img DISPLAY_LIBRARY=$(DISPLAY_LIBRARY) CYL=128 LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libXpm.so.4 bochs -f etc/bochsrc

out/%.o: src/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

out/%.o: src/%.asm
	@mkdir -p out/
	nasm $< -felf64 -o $@

kernel.sys: $(OBJS)
	ld -Tsrc/kernel.ld -melf_x86_64 -o /tmp/kernel $^
	cat bootloader/pure64.sys /tmp/kernel > kernel.sys

disk.img: kernel.sys
	./createimage.sh disk.img 1 bootloader/bmfs_mbr.sys kernel.sys

clean:
	rm -f disk.img entry.sys kernel.sys out/*

-include out/*.d

.PHONY: run clean
