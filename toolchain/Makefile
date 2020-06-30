create_floppy_boot:
	gcc create_floppy_boot.c ../fs/fat12/fat12.c -o ../bin/create_floppy_boot
	cp floppy_boot_linux.bxrc.in ../bin/floppy_boot_linux.bxrc
	echo "#!/bin/bash\nbochs -f floppy_boot_linux.bxrc" > ../bin/floppy_debug.sh