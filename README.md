[![Build Status](https://travis-ci.org/gnarlie/bad-os-64.png?branch=master)](https://travis-ci.org/gnarlie/bad-os-64)
Bad-OS-64
=========

This is a simple x86-64 kernel.

Requirements
------------
You'll need bochs (or a similar emulator), nasm, along with the usual GCC stack.

    apt-get install nasm build-essential bochs bochs-sdl

To run inside bochs, just use:

    make run

To run inside QEMU, just use:

    qemu-system-x86_64 --readconfig etc/qemu.cfg

