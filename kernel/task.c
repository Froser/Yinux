#include <yinux/task.h>
#include <yinux/cpu.h>
#include <yinux/trap.h>
#include <yinux/memory.h>

#define load_TR(n)                        \
do {                                      \
    __asm__ __volatile__( "ltr  %%ax"     \
        :                                 \
        :"a"(n << 3)                      \
        :"memory");                       \
} while(0)

#define INIT_TASK_RSP0 ((unsigned long)(g_init_task_u.stack + STACK_SIZE / sizeof(unsigned long)))

#define INIT_TASK(tsk)                  \
{                                       \
    .state = task_uninterruptible,      \
    .flags = pf_kthread,                \
    .mm = &g_init_mm,                   \
    .addr_limit = 0xffff800000000000,   \
    .pid = 0,                           \
    .counter = 1,                       \
    .signal = 0,                        \
    .priority = 0                       \
}

#define INIT_TSS                        \
{                                       \
    .reserved0 = 0,                     \
    .rsp0 = INIT_TASK_RSP0,             \
    .rsp1 = INIT_TASK_RSP0,             \
    .rsp2 = INIT_TASK_RSP0,             \
    .reserved1 = 0,                     \
    .ist1 = 0xffff800000007c00,         \
    .ist2 = 0xffff800000007c00,         \
    .ist3 = 0xffff800000007c00,         \
    .ist4 = 0xffff800000007c00,         \
    .ist5 = 0xffff800000007c00,         \
    .ist6 = 0xffff800000007c00,         \
    .ist7 = 0xffff800000007c00,         \
    .reserved2 = 0,                     \
    .reserved3 = 0,                     \
    .iomapbaseaddr = 0                  \
}

mm_struct g_init_mm = { 0 };

task_u g_init_task_u __attribute__((__section__ (".data.init_task"))) = {
    INIT_TASK(g_init_task_u.task)
};

thread g_init_thread = {
    .rsp0 = INIT_TASK_RSP0,
    .rsp = INIT_TASK_RSP0,
    .fs = KERNEL_DS,
    .gs = KERNEL_DS,
    .cr2 = 0,
    .trap_nr = 0,
    .error_code = 0
};

tss g_init_tss[NR_CPUS] = { [0 ... NR_CPUS - 1] = INIT_TSS };

task* get_current_task()
{
    task* current = NULL;
    __asm__ __volatile__ ("andq %%rsp, %0 \n\t":"=r"(current):"0"(~32767UL)); /* ~32767UL = 0xffffffffffff8000 */
    return current;
}

#define switch_to (prev, next)                                                                      \
do {                                                                                                \
    __asm__ __volatile__ (                                                                          \
                            "pushq      %%rbp           \n\t"                                       \
                            "pushq      %%rax           \n\t"                                       \
                            "movq       %%rsp, %0       \n\t"                                       \
                            "movq       %2, %%rsp       \n\t"                                       \
                            "leaq       1f(%%rip), %%rax\n\t"                                       \
                            "movq       %%rax, %1       \n\t"                                       \
                            "pushq      %3              \n\t"                                       \
                            "jmp        __switch_to     \n\t"                                       \
                            "1:                         \n\t"                                       \
                            "popq       %%rax           \n\t"                                       \
                            "popq       %%rbp           \n\t"                                       \
                            :"=m"(prev->thread->rsp),"=m"(prev->thread->rip)                        \
                            :"m"(next->thread->rsp),"m"(next->thread->rip), "D"(prev), "S"(next)    \
                            :"memory"                                                               \
                        );                                                                          \
}

/* RDI = prev, RSI = next */
void __switch_to(task* prev, task* next)
{
    g_init_tss[0].rsp0 = next->thread->rsp0;
    set_tss64(&g_init_tss[0], g_init_tss[0].rsp0, g_init_tss[0].rsp1, g_init_tss[0].rsp2,
        g_init_tss[0].ist1, g_init_tss[0].ist2, g_init_tss[0].ist3, g_init_tss[0].ist4,
        g_init_tss[0].ist5, g_init_tss[0].ist6, g_init_tss[0].ist7);
    __asm__ __volatile__ ("movq %%fs, %0 \n\t":"=a"(prev->thread->fs));
    __asm__ __volatile__ ("movq %%gs, %0 \n\t":"=a"(prev->thread->gs));
    __asm__ __volatile__ ("movq %0, %%gs \n\t"::"a"(next->thread->gs));
    __asm__ __volatile__ ("movq %0, %%fs \n\t"::"a"(next->thread->gs));
}

/* Must be invoked after sys_memory_init  */
extern unsigned long _stack_start;
void sys_task_init()
{
    load_TR(8);
    task* p = NULL;
    const Global_Memory_Descriptor* gmd = get_kernel_memdesc();
    g_init_mm.pml4t = get_kernel_CR3();
    g_init_mm.start_code = gmd->start_code;
    g_init_mm.end_code = gmd->end_code;
    g_init_mm.start_data = gmd->start_data;
    g_init_mm.end_data = gmd->end_data;
    g_init_mm.start_rodata = gmd->start_rodata;
    g_init_mm.end_rodata = gmd->end_rodata;
    g_init_mm.start_brk = 0;
    g_init_mm.end_brk = gmd->end_brk;
    g_init_mm.start_stack = _stack_start;
    set_tss64(&g_init_tss[0], g_init_thread.rsp0, g_init_tss[0].rsp1, g_init_tss[0].rsp2,
        g_init_tss[0].ist1, g_init_tss[0].ist2, g_init_tss[0].ist3, g_init_tss[0].ist4,
        g_init_tss[0].ist5, g_init_tss[0].ist6, g_init_tss[0].ist7);
    g_init_tss[0].rsp0 = g_init_thread.rsp0;
    //...
}