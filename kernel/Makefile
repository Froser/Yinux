GCC_FLAGS := -mcmodel=large -fno-builtin -m64 -I ../include

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ../bin/system ../bin/kernel.bin

system: head.o main.o trap.o entry.o
	ld -b elf64-x86-64 -o ../bin/system ../bin/head.o ../bin/main.o ../bin/trap.o ../bin/entry.o -T Kernel.lds

main.o: main.c
	gcc $(GCC_FLAGS) -c main.c -o ../bin/main.o

trap.o: trap.c
	gcc $(GCC_FLAGS) -c trap.c -o ../bin/trap.o

head.o: head.S
	gcc -E head.S > ../bin/head.S
	as --64 -o ../bin/head.o ../bin/head.S

entry.o: entry.S
	gcc -E entry.S > ../bin/entry.S
	as --64 -o ../bin/entry.o ../bin/entry.S