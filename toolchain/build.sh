#!/bin/bash
mkdir -p ../bin

cd ./freetype-2.10.2
make
cd ..

make

echo "Done."