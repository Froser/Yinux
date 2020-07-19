OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
	. = 0xffff800000000000 + 0x100000;

	.data.kernel :
	{
		_data.kernel = .;
		*(.data.kernel)
		_edata.kernel = .;
	}

	. = 0xffff800000000000 + 0x106000;

	.text :
	{
		_text = .;
		*(.text)
		_etext = .;
	}

	.data :
	{
		_data = .;
		*(.data)
		_edata = .;
	}

	. = ALIGN(8);

	.got :
	{
		_got = .;
		*(.got)
		_egot = .;
	}

	.got.plt :
	{
		_got.plt = .;
		*(.got.plt)
		_egot.plt = .;
	}

	.rodata :
	{
		_rodata = .;
		*(.rodata)
		_erodata = .;
	}

	. = ALIGN(32768);

	.data.init_task :
	{
		*(.data.init_task)
	}

	.bss :
	{
		_bss = .;
		*(.bss)
		_ebss = .;
	}

	_end = .;
}