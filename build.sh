#!/bin/bash

cd toolchain
bash ./build.sh
cd ..

cd boot
bash ./build.sh
cd ..

make