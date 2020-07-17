#include "trap.h"
#include <yinux/stringify.h>
#include <yinux/kernel.h>

#define SAVE_ALL                                    \
    "cld;                                    \n\t"  \
    "pushq %rax                              \n\t"  \
    "pushq %rax                              \n\t"  \
    "movq  %es, %rax                         \n\t"  \
    "pushq %rax                              \n\t"  \
    "movq %ds, %rax                          \n\t"  \
    "pushq %rax                              \n\t"  \
    "xorq %rax, %rax                         \n\t"  \
    "pushq %rbp                              \n\t"  \
    "pushq %rdi                              \n\t"  \
    "pushq %rsi                              \n\t"  \
    "pushq %rdx                              \n\t"  \
    "pushq %rcx                              \n\t"  \
    "pushq %rbx                              \n\t"  \
    "pushq %r8                               \n\t"  \
    "pushq %r9                               \n\t"  \
    "pushq %r10                              \n\t"  \
    "pushq %r11                              \n\t"  \
    "pushq %r12                              \n\t"  \
    "pushq %r13                              \n\t"  \
    "pushq %r14                              \n\t"  \
    "pushq %r15                              \n\t"  \
    "movq  $0x10, %rdx                       \n\t"  \
    "movq  %rdx, %ds                         \n\t"  \
    "movq  %rdx, %es                         \n\t"

#define IRQ_NAME_(nr) nr##_interrupt(void)
#define IRQ_NAME(nr)   IRQ_NAME_(IRQ##nr)
#define IRQ_IMPL(nr)   IRQ##nr##_interrupt
#define IRQ_ENTRY(nr)                               \
    void IRQ_NAME(nr);                              \
    __asm__(                                        \
        __stringify(IRQ)#nr"_interrupt:      \n\t"  \
        "pushq $0x00                         \n\t"  \
        SAVE_ALL                                    \
        "movq %rsp, %rdi                     \n\t"  \
        "leaq ret_from_intr(%rip), %rax      \n\t"  \
        "push %rax                           \n\t"  \
        "movq $"#nr", %rsi                   \n\t"  \
        "jmp  do_IRQ                         \n\t"  \
        );

void do_IRQ(unsigned long rsp, unsigned long nr)
{
    printk("interrupt %ld\n", nr);
    io_out8(0x20, 0x20);
    while(1);
}

/* interrupt handlers */
IRQ_ENTRY(0x20)
IRQ_ENTRY(0x21)
IRQ_ENTRY(0x22)
IRQ_ENTRY(0x23)
IRQ_ENTRY(0x24)
IRQ_ENTRY(0x25)
IRQ_ENTRY(0x26)
IRQ_ENTRY(0x27)
IRQ_ENTRY(0x28)
IRQ_ENTRY(0x29)
IRQ_ENTRY(0x30)
IRQ_ENTRY(0x31)
IRQ_ENTRY(0x32)
IRQ_ENTRY(0x33)
IRQ_ENTRY(0x34)
IRQ_ENTRY(0x35)
IRQ_ENTRY(0x36)
IRQ_ENTRY(0x37)

void (*interrupt[24])(void) = {
    IRQ_IMPL(0x20),
    IRQ_IMPL(0x21),
    IRQ_IMPL(0x22),
    IRQ_IMPL(0x23),
    IRQ_IMPL(0x24),
    IRQ_IMPL(0x25),
    IRQ_IMPL(0x26),
    IRQ_IMPL(0x27),
    IRQ_IMPL(0x28),
    IRQ_IMPL(0x29),
    IRQ_IMPL(0x30),
    IRQ_IMPL(0x31),
    IRQ_IMPL(0x32),
    IRQ_IMPL(0x33),
    IRQ_IMPL(0x34),
    IRQ_IMPL(0x35),
    IRQ_IMPL(0x36),
    IRQ_IMPL(0x37)
};

void sys_8259A_init()
{
    /* device interrupt vectors start from 32 */
    for (int i = 32; i < 56; ++i) {
        set_intr_gate(i, 2, interrupt[i - 32]);
    }

    /* 8259A-master ICW1-4 */
    io_out8(0x20, 0x11);
    io_out8(0x21, 0x20);
    io_out8(0x21, 0x04);
    io_out8(0x21, 0x01);

    /* 8259A-slave ICW1-4 */
    io_out8(0xa0, 0x11);
    io_out8(0xa1, 0x28);
    io_out8(0xa1, 0x02);
    io_out8(0xa1, 0x01);

    /* 8259A-M/S OCW1 */
    io_out8(0x21, 0xfd); /* test only keyboard */
    io_out8(0xa1, 0xff);

    printk("Interrupt inited.\n");
}