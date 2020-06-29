GCC_FLAGS := -mcmodel=large -fno-builtin -m64

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ../bin/system ../bin/kernel.bin

system: head.o main.o
	ld -b elf64-x86-64 -o ../bin/system ../bin/head.o ../bin/main.o -T Kernel.lds

main.o: main.c
	gcc $(GCC_FLAGS) -c main.c -o ../bin/main.o

head.o: head.S
	gcc -E head.S > ../bin/head.S
	as --64 -o ../bin/head.o ../bin/head.S