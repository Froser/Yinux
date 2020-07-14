#!/bin/bash

cd toolchain
bash ./build.sh
cd ..

cd boot
bash ./build.sh
cd ..

make

FILE=./bin/create_floppy_boot
if test -f "$FILE"; then
    ./bin/create_floppy_boot ./bin/boot.bin -i ./bin/loader.bin loader.bin -i ./bin/kernel.bin kernel.bin -o ./bin/boot.img
else
    echo "$FILE not exists. Please build create_floppy_boot first."
fi

cp ./toolchain/floppy_boot_linux.bxrc.in ./bin/floppy_boot_linux.bxrc