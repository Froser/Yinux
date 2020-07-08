#include <yinux/trap.h>

void Kernel_Main()
{
	sys_vector_init();
	while (1);
}