#! /bin/bash

set -e

function die() {
    echo $* 1>&2
    exit 1
}

[ 4 -eq $# ] || die "usage: " $0 " img-file img-megs stage1 stage2 "

dd if=/dev/zero of=$1 bs=1M count=$2 conv=sparse
dd if=$3 of=$1 seek=0 conv=notrunc bs=512
dd if=$4 of=$1 bs=512 seek=16 conv=notrunc
