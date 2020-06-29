; 内核代码
.section .data

.globl GDT_TABLE

GDT_TABLE:
	.quad			0x0000000000000000			/* NULL描述符 */

GDT_END