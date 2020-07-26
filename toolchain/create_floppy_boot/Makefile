all: create_floppy_boot
	cp create_floppy_boot/floppy_boot_linux.bxrc.in ../bin/floppy_boot_linux.bxrc
	../bin/create_floppy_boot ../bin/boot.bin -i ../bin/loader.bin loader.bin -i ../bin/kernel.bin kernel.bin -o ../bin/boot.img
	echo "#!/bin/bash\nbochs -f floppy_boot_linux.bxrc" > ../bin/floppy_debug.sh

create_floppy_boot:
	gcc create_floppy_boot/create_floppy_boot.c ../fs/fat12/fat12.c -o ../bin/create_floppy_boot