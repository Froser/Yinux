#include "memory.h"

void init_memory()
{
    Memory_E820_Formate* mem = (Memory_E820_Formate*)0xffff800000007e00;
    printk(KERN_INFO "Memory details:\n");
    while (1);
}