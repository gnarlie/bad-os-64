run: disk.img
	IMAGE=disk.img CYL=128 bochs -f etc/bochsrc

entry.sys:
	nasm src/entry.asm -o entry.sys

kernel.sys: entry.sys
	cat bootloader/pure64.sys entry.sys > kernel.sys

disk.img: kernel.sys
	sudo ./createimage.sh disk.img 1 bootloader/bmfs_mbr.sys kernel.sys

clean:
	rm -f disk.img entry.sys kernel.sys

.PHONY: run clean
