#include <yinux/desc_defs.h>
#include <yinux/kernel.h>
#include "trap.h"

#define load_TR(n)                        \
do {                                      \
    __asm__ __volatile__( "ltr  %%ax"     \
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

#define NOT_IMPLEMENT_HANDLER(name)                                         \
    void name();                                                            \
    void do_##name(unsigned long rsp, unsigned long error_code) {           \
        printk("The method " #name " is not implement. Error code: %ld\n", error_code); \
        while (1);                                                          \
    }

NOT_IMPLEMENT_HANDLER(divide_error);
NOT_IMPLEMENT_HANDLER(debug);
NOT_IMPLEMENT_HANDLER(nmi);
NOT_IMPLEMENT_HANDLER(breakpoint);
NOT_IMPLEMENT_HANDLER(overflow);
NOT_IMPLEMENT_HANDLER(bound_range_excceeded);
NOT_IMPLEMENT_HANDLER(invalid_opcode);
NOT_IMPLEMENT_HANDLER(device_not_available)
NOT_IMPLEMENT_HANDLER(double_fault);
NOT_IMPLEMENT_HANDLER(invalid_tss);
NOT_IMPLEMENT_HANDLER(segment_not_present);
NOT_IMPLEMENT_HANDLER(stack_segment_fault);
NOT_IMPLEMENT_HANDLER(general_protection);
NOT_IMPLEMENT_HANDLER(x87_floating_point_exception);
NOT_IMPLEMENT_HANDLER(alignment_check);
NOT_IMPLEMENT_HANDLER(machine_check);

extern gate_struct64 IDT_Table[];

void set_intr_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n, 0x8E, ist, addr);  /* P,DPL=0, TYPE=E */
}

void set_trap_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n, 0x8F, ist, addr);  /* P,DPL=0, TYPE=F */
}

void set_system_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n, 0xEF, ist, addr);  /* P,DPL=3, TYPE=F */
}

/* intr handlers */
void int_failure()
{
    printk("Unknown interrupt or fault.\n");
    while (1);
}

void page_fault();
void do_page_fault(unsigned long rsp, unsigned long error_code)
{
    unsigned long *p = 0;
    unsigned long cr2 = 0;
    __asm__ __volatile__ ("movq %%cr2, %0":"=r"(cr2)::"memory");
    p = (unsigned long*)(rsp + 0x98);  /* RIP */
    printk("Page fault at %#018lx\n", cr2);
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
    set_trap_gate(X86_TRAP_DB, 1, debug);
    set_intr_gate(X86_TRAP_NMI,1, nmi);
    set_system_gate(X86_TRAP_BP, 1, breakpoint);
    set_system_gate(X86_TRAP_OF, 1, overflow);
    set_system_gate(X86_TRAP_BR, 1, bound_range_excceeded);
    set_trap_gate(X86_TRAP_UD, 1, invalid_opcode);
    set_trap_gate(X86_TRAP_NM, 1, device_not_available);
    set_trap_gate(X86_TRAP_DF, 1, double_fault);
    set_trap_gate(X86_TRAP_TS, 1, invalid_tss);
    set_trap_gate(X86_TRAP_NP, 1, segment_not_present);
    set_trap_gate(X86_TRAP_SS, 1, stack_segment_fault);
    set_trap_gate(X86_TRAP_GP, 1, general_protection);
    set_trap_gate(X86_TRAP_PF, 1, page_fault);
    set_trap_gate(X86_TRAP_MF, 1, x87_floating_point_exception);
    set_trap_gate(X86_TRAP_AC, 1, alignment_check);
    set_trap_gate(X86_TRAP_MC, 1, machine_check);
}