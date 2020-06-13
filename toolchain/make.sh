#!/bin/bash
mkdir -p ../bin

echo "Compiling create_floppy_boot..."
gcc create_floppy_boot.c -o ../bin/create_floppy_boot

echo "Done."