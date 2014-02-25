CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -std=gnu99  -MMD -MP -g -Werror
TEST_CFLAGS=-std=gnu99 -Werror -MMD -MP -I src
DISPLAY_LIBRARY ?= sdl

C_FILES=$(shell find src -name '*.c')
ASM_FILES=$(shell find src -name '*.asm')
TEST_SRC=$(shell find test -name '*.c')

OBJS=$(patsubst src/%.asm, out/%.o, $(ASM_FILES)) $(patsubst src/%.c, out/%.o, $(C_FILES))
TEST_OBJS=$(patsubst test/%.c, out/test/%.o, $(TEST_SRC))

default: kernel.sys

test: bin/alltests
	bin/alltests

.PHONY : test

bin/alltests: $(TEST_OBJS) | $(filter-out out/entry.o out/panic.o out/main.o out/interrupt.o, $(OBJS))
	$(CC) -o $@ $^ $|

run: disk.img
	IMAGE=disk.img DISPLAY_LIBRARY=$(DISPLAY_LIBRARY) CYL=128 LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libXpm.so.4 bochs -f etc/bochsrc

out/%.o: src/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

out/%.o: src/%.asm
	@mkdir -p out/
	nasm $< -felf64 -o $@

out/test/%.o: test/%.c
	@mkdir -p out/test
	$(CC) -c $(TEST_CFLAGS) -o $@ $<


kernel.sys: $(OBJS)
	ld -Tsrc/kernel.ld -melf_x86_64 -o /tmp/kernel $^
	cat bootloader/pure64.sys /tmp/kernel > kernel.sys

disk.img: kernel.sys
	./createimage.sh disk.img 1 bootloader/bmfs_mbr.sys kernel.sys

clean:
	rm -rf disk.img entry.sys kernel.sys out/* bin/*

-include out/*.d

.PHONY: run clean
