#!/bin/bash
mkdir -p ../bin

echo "Compiling floppy_boot.S..."
nasm floppy_boot.S -o ../bin/boot.bin

echo "Compiling loader.S..."
nasm loader.S -o ../bin/loader.bin

echo "Compiling kernel.S..."
nasm kernel.S -o ../bin/kernel.bin

FILE=../bin/create_floppy_boot
if test -f "$FILE"; then
	../bin/create_floppy_boot ../bin/boot.bin -i ../bin/loader.bin loader.bin -i ../bin/kernel.bin kernel.bin -o ../bin/boot.img
else
    echo "$FILE not exists. Please make create_floppy_boot first."
fi