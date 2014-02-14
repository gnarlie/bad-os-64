#! /bin/bash

function die() {
    echo $* 1>&2
    exit
}

[ 0 -eq $(id -u) ] || die "You need to be root"
[ 4 -eq $# ] || die "usage: " $0 " img-file img-megs stage1 stage2 "

losetup -d /dev/loop0

dd if=/dev/zero of=$1 bs=1M count=$2
losetup /dev/loop0 $1
dd if=$3 of=/dev/loop0 bs=512
dd if=$4 of=/dev/loop0 bs=512 seek=16
