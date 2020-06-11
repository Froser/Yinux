#!/bin/bash
mkdir -p ../bin

echo "Building boot.img..."
dd if=/dev/zero of=../bin/boot.img bs=512 count=2880

echo "Compiling floppy_boot.S..."
nasm floppy_boot.S -o ../bin/boot.bin

echo "Writing to boot.img..."
dd if=../bin/boot.bin of=../bin/boot.img bs=512 count=1 conv=notrunc

echo "Compiling loader.S..."
nasm loader.S -o ../bin/loader.bin

echo "Mounting loader.bin..."
mount ../bin/boot.img /media/ -t vfat -o -loop
cp ../bin/loader.bin /media/
sync
umount /media/

echo "Done."