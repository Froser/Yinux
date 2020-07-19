#include <yinux/trap.h>
#include <yinux/kernel.h>
#include <yinux/memory.h>
#include <yinux/task.h>

void Kernel_Main()
{
    init_printk();
    printk(KERN_INFO "Welcome to Yinux!\n");
    
    sys_vector_init();
    sys_memory_init();
    sys_8259A_init();
    sys_task_init();
    sti();
    while (1);
}