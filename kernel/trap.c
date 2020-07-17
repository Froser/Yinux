#include <yinux/desc_defs.h>
#include <yinux/kernel.h>
#include "trap.h"

#define load_TR(n)                        \
do {                                      \
  __asm__ __volatile__( "ltr  %%ax"       \
        :                                 \
        :"a"(n << 3)                      \
        :"memory");                       \
} while(0)

void set_tss64(unsigned int * Table,unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{
  *(unsigned long *)(Table+1) = rsp0;
  *(unsigned long *)(Table+3) = rsp1;
  *(unsigned long *)(Table+5) = rsp2;

  *(unsigned long *)(Table+9) = ist1;
  *(unsigned long *)(Table+11) = ist2;
  *(unsigned long *)(Table+13) = ist3;
  *(unsigned long *)(Table+15) = ist4;
  *(unsigned long *)(Table+17) = ist5;
  *(unsigned long *)(Table+19) = ist6;
  *(unsigned long *)(Table+21) = ist7;
}

/* Set gate macro */
/* rdi = IDT_Table + n, rdx = code_attr, rcx = ist */
#define _set_gate(gate_selector_addr, attr, ist, code_attr)                 \
do                                                                          \
{                                                                           \
    unsigned long __d0, __d1;                                               \
    __asm__ __volatile__ ( "movw   %%dx,   %%ax     \n\t "                  \
                           "andq   $0x7,   %%rcx    \n\t "                  \
                           "addq   %4,     %%rcx    \n\t "                  \
                           "shlq   $32,    %%rcx    \n\t "                  \
                           "addq   %%rcx,  %%rax    \n\t "                  \
                           "xorq   %%rcx,  %%rcx    \n\t "                  \
                           "movl   %%edx,  %%ecx    \n\t "                  \
                           "shrq   $16,    %%rcx    \n\t "                  \
                           "shlq   $48,    %%rcx    \n\t "                  \
                           "addq   %%rcx,  %%rax    \n\t "                  \
                           "movq   %%rax,  %0       \n\t "                  \
                           "shrq   $32,    %%rdx    \n\t "                  \
                           "movq   %%rdx,  %1       \n\t "                  \
                           "andq   $0x7,   %%rcx    \n\t "                  \
                           :"=m"(*((unsigned long*)(gate_selector_addr))),  \
                           "=m"(*(1+(unsigned long*)(gate_selector_addr))), \
                           "=&a"(__d0),"=&d"(__d1)                          \
                           :"i"(attr << 8),                                 \
                           "3"((unsigned long *)(code_attr)),               \
                           "2"(0x8 << 16),                                  \
                           "c"(ist)                                         \
                           :"memory"                                        \
                           );                                               \
} while(0)

void divide_error();
void nmi();
void general_protection();
void page_fault();

extern gate_struct64 IDT_Table[];

void set_intr_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n, 0x8E, ist, addr);  /* P,DPL=0, TYPE=E */
}

void set_trap_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n, 0x8F, ist, addr);  /* P,DPL=0, TYPE=F */
}

/* intr handlers */
void int_failure()
{
    printk("Unknown interrupt or fault.\n");
    while (1);
}

void do_divide_error(unsigned long rsp, unsigned long error_code)
{
    while (1);
}

void do_nmi(unsigned long rsp, unsigned long error_code)
{
    while (1);
}

void do_page_fault(unsigned long rsp, unsigned long error_code)
{
    unsigned long *p = 0;
    unsigned long cr2 = 0;
    __asm__ __volatile__ ("movq %%cr2, %0":"=r"(cr2)::"memory");
    p = (unsigned long*)(rsp + 0x98);  /* RIP */
    while (1);
}

void do_general_protection(unsigned long rsp,unsigned long error_code)
{
    printk("General protection fault.\n");
    while (1);
}

void sys_tss_init()
{
    load_TR(8); /* ltr $40 */
    set_tss64(0, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
      0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);
}

void sys_vector_init()
{
    set_trap_gate(X86_TRAP_DE, 1, divide_error);
    set_intr_gate(X86_TRAP_NMI,1, nmi);
    set_trap_gate(X86_TRAP_GP, 1, general_protection);
    set_trap_gate(X86_TRAP_PF, 1, page_fault);
}