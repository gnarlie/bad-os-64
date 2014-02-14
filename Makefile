CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector

run: disk.img
	IMAGE=disk.img CYL=128 LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libXpm.so.4 bochs -f etc/bochsrc

out/%.o: src/%.c
	$(CC) -c $(CFLAGS) -o $@ $^

out/%.o: src/%.asm
	nasm $< -felf64 -o $@


kernel.sys: out/entry.o out/main.o
	ld -Tsrc/kernel.ld -melf_x86_64 -o /tmp/kernel $^
	cat bootloader/pure64.sys /tmp/kernel > kernel.sys

disk.img: kernel.sys
	sudo ./createimage.sh disk.img 1 bootloader/bmfs_mbr.sys kernel.sys

clean:
	rm -f disk.img entry.sys kernel.sys out/*

.PHONY: run clean
