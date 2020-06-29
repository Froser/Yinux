#!/bin/bash

cd toolchain
bash ./make.sh
cd ..

cd boot
bash ./make.sh
cd ..

cd kernel
bash ./make.sh
cd ..

FILE=./bin/create_floppy_boot
if test -f "$FILE"; then
    ./bin/create_floppy_boot ./bin/boot.bin -i ./bin/loader.bin loader.bin -i ./bin/kernel.bin kernel.bin -o ./bin/boot.img
else
    echo "$FILE not exists. Please make create_floppy_boot first."
fi
