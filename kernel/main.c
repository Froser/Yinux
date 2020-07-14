#include <yinux/trap.h>
#include <yinux/kernel.h>
#include <yinux/memory.h>

void Kernel_Main()
{
	init_printk();
	printk("Welcome to Yinux!\n");
    sys_vector_init();
    init_memory();
    while (1);
}