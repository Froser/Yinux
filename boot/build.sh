#!/bin/bash
mkdir -p ../bin

echo "Compiling floppy_boot.S..."
nasm floppy_boot.asm -o ../bin/boot.bin

echo "Compiling loader.S..."
nasm loader.asm -o ../bin/loader.bin