#!/bin/bash
# 1. Create a new file system on a file over a ramfs
# 2. Using hdstress to test the filesystem

set -e
set -x

BIN=$PWD/../src
TARGET=./target   # place where to mount the filesystem
TMPFS=./tmpfs
SIZE=100M

[ ! -d $TMPFS ] && mkdir $TMPFS
[ ! -d $TARGET ] && mkdir $TARGET

sudo mount -t tmpfs -o size=$SIZE tmpfs $TMPFS

truncate -s $SIZE $TMPFS/nanofs.raw
$BIN/mkfs.nanofs -v $TMPFS/nanofs.raw

# Test single thread
$BIN/nanofuse $TMPFS/nanofs.raw $TARGET
python3 hdstress/hdstress -s 1 $TARGET
fusermount -u $TARGET

# Test several threads
$BIN/nanofuse $TMPFS/nanofs.raw $TARGET
python3 hdstress/hdstress -j 4 -s 1 $TARGET
fusermount -u $TARGET


sudo umount $TMPFS
