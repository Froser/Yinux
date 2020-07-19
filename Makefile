GCC_FLAGS := -mcmodel=large -fno-builtin -fno-stack-protector -m64 -I ./include

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary ./bin/system ./bin/kernel.bin

system: head.o main.o trap.o memory.o entry.o ctype.o string.o vsprintf.o printk.o interrupt.o task.o
	ld -b elf64-x86-64 -o ./bin/system ./bin/head.o ./bin/ctype.o ./bin/interrupt.o ./bin/task.o ./bin/string.o ./bin/vsprintf.o ./bin/printk.o ./bin/main.o ./bin/memory.o ./bin/trap.o ./bin/entry.o -T ./kernel/Kernel.lds
	objdump -D ./bin/system > ./bin/system.output.txt

main.o: ./kernel/main.c
	gcc $(GCC_FLAGS) -c ./kernel/main.c -o ./bin/main.o

trap.o: ./kernel/trap.c
	gcc $(GCC_FLAGS) -c ./kernel/trap.c -o ./bin/trap.o

memory.o: ./kernel/memory.c
	gcc $(GCC_FLAGS) -c ./kernel/memory.c -o ./bin/memory.o

printk.o: ./kernel/printk.c
	gcc $(GCC_FLAGS) -c ./kernel/printk.c -o ./bin/printk.o

interrupt.o: ./kernel/interrupt.c
	gcc $(GCC_FLAGS) -c ./kernel/interrupt.c -o ./bin/interrupt.o

task.o: ./kernel/task.c
	gcc $(GCC_FLAGS) -c ./kernel/task.c -o ./bin/task.o	

vsprintf.o: ./lib/vsprintf.c
	gcc $(GCC_FLAGS) -c ./lib/vsprintf.c -o ./bin/vsprintf.o

string.o: ./lib/string.c
	gcc $(GCC_FLAGS) -c ./lib/string.c -o ./bin/string.o

ctype.o: ./lib/ctype.c
	gcc $(GCC_FLAGS) -c ./lib/ctype.c -o ./bin/ctype.o

head.o: ./kernel/head.S
	gcc -E ./kernel/head.S > ./bin/head.S
	as --64 -o ./bin/head.o ./bin/head.S

entry.o: ./kernel/entry.S
	gcc -E ./kernel/entry.S > ./bin/entry.S -I ../include
	as --64 -o ./bin/entry.o ./bin/entry.S -I ../include