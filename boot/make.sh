#!/bin/bash
echo "Building boot.img..."
dd if=/dev/zero of=boot.img bs=512 count=2880

echo "Compiling floppy_boot.S..."
nasm floppy_boot.S -o boot.bin

echo "Writing to boot.img..."
dd if=boot.bin of=boot.img bs=512 count=1 conv=notrunc

echo "Cleaning..."
rm boot.bin

echo "Done."