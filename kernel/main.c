#include <yinux/trap.h>
#include <yinux/kernel.h>
#include <yinux/memory.h>

void Kernel_Main()
{
	init_printk();
	printk(KERN_INFO "Welcome to Yinux!\n");
	
	sys_tss_init();
    sys_vector_init();
    sys_init_memory();
    sys_interrupt_init();
    while (1);
}