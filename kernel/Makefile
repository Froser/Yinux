all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

system: head.o
	ld -b elf64-x86-64 -z muldefs -o system head.o

head.o: head.S
	gcc -E head.S > ../bin/head.S
	as --64 -o ../bin/head.o ../bin/head.S