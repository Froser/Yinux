#include <yinux/desc_defs.h>
#include "trap.h"

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
                           "andq   %%rcx,  %%rax    \n\t "                  \
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

extern gate_struct64 IDT_Table[];

void set_intr_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n, 0x8E, ist, addr);  /* P,DPL=0, TYPE=E */
}

void set_trap_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n, 0x8E, ist, addr);  /* P,DPL=0, TYPE=E */
}

/* intr handlers */
void do_divide_error(unsigned long rsp, unsigned long error_code)
{
    while (1);
}

void sys_vector_init()
{
    set_trap_gate(X86_TRAP_DE, 1, divide_error);
}