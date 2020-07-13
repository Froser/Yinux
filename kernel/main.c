#include <yinux/trap.h>
#include "memory.h"

void Kernel_Main()
{
	sys_vector_init();
	init_memory();
	while (1);
}