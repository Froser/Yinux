#!/bin/bash
mkdir -p ../bin

echo "Compiling floppy_boot.S..."
nasm floppy_boot.S -o ../bin/boot.bin

echo "Compiling loader.S..."
nasm loader.S -o ../bin/loader.bin

FILE=../bin/create_floppy_boot
if test -f "$FILE"; then
	../bin/create_floppy_boot ../bin/boot.bin ../bin/loader.bin ../bin/boot.img
else
    echo "$FILE not exists. Please make create_floppy_boot first."
fi

echo
echo "Done."