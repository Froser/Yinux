#!/bin/bash

cd toolchain
./make.sh
cd ..

cd boot
./make.sh
cd ..