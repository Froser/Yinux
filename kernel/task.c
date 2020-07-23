#include <yinux/task.h>
#include <yinux/cpu.h>
#include <yinux/trap.h>
#include <yinux/memory.h>
#include <yinux/ptrace.h>
#include <yinux/string.h>
#include <yinux/list.h>
#include <yinux/sched.h>

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

void printk_tss(const TSS* tss) {
    printk(KERN_INFO "TSS of %p details:\nrsp0=%p\nrsp1=%p\nrsp2=%p\nist1=%p\nist2=%p\nrsp3=%p\nrsp4=%p\nrsp5=%p\nrsp6=%p\nrsp7=%p\n",
        tss,
        to_pointer(tss->rsp0), to_pointer(tss->rsp1), to_pointer(tss->rsp2),
        to_pointer(tss->ist1), to_pointer(tss->ist2), to_pointer(tss->ist3),
        to_pointer(tss->ist4), to_pointer(tss->ist5), to_pointer(tss->ist6), to_pointer(tss->ist7)
        );
}

task_u g_init_task_u __attribute__((__section__ (".data.init_task"))) = {
    INIT_TASK(g_init_task_u.task)
};

Thread g_init_thread = {
    .rsp0 = INIT_TASK_RSP0,
    .rsp = INIT_TASK_RSP0,
    .fs = KERNEL_DS,
    .gs = KERNEL_DS,
    .cr2 = 0,
    .trap_nr = 0,
    .error_code = 0
};

TSS g_init_tss[NR_CPUS] = { [0 ... NR_CPUS - 1] = INIT_TSS };

Task* get_current_task()
{
    Task* current = NULL;
    __asm__ __volatile__ ("andq %%rsp, %0 \n\t":"=r"(current):"0"(~32767UL)); /* ~32767UL = 0xffffffffffff8000 */
    return current;
}

/* exchange rsp, record rip */
#define switch_to(prev, next)                                                                       \
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
} while (0);

/* RDI = prev, RSI = next */
void __switch_to(Task* prev, Task* next)
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

unsigned long do_exit(unsigned long code)
{
    printk("task exited with result code %lu", code);
    while(1);
}

/* Create process */
void ret_from_intr();
void kernel_thread_func();
__asm__ (
"kernel_thread_func:        \n\t"
"   popq    %r15            \n\t"
"   popq    %r14            \n\t"   
"   popq    %r13            \n\t"   
"   popq    %r12            \n\t"   
"   popq    %r11            \n\t"   
"   popq    %r10            \n\t"   
"   popq    %r9             \n\t"   
"   popq    %r8             \n\t"   
"   popq    %rbx            \n\t"   
"   popq    %rcx            \n\t"   
"   popq    %rdx            \n\t"   
"   popq    %rsi            \n\t"   
"   popq    %rdi            \n\t"   
"   popq    %rbp            \n\t"   
"   popq    %rax            \n\t"   
"   movq    %rax,   %ds     \n\t"
"   popq    %rax            \n\t"
"   movq    %rax,   %es     \n\t"
"   popq    %rax            \n\t"
"   addq    $0x38,  %rsp    \n\t"
/////////////////////////////////
"   movq    %rdx,   %rdi    \n\t"
"   callq   *%rbx           \n\t"
"   movq    %rax,   %rdi    \n\t"
"   callq   do_exit         \n\t"
);

unsigned long init(unsigned long arg)
{
    return 1;
}

unsigned long do_fork(pt_regs* regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size)
{
    Memory_Page* p = alloc_pages(zone_normal, 1, PG_PTable_Mapped | PG_Kernel);
    Task* task = (Task*)MEM_P2V(p->phy_addr);
    memset(task, 0, sizeof(*task));
    *task = *get_current_task();
    list_init(&task->list);
    list_push_front(&g_init_task_u.task.list, &task->list); /* init -> task */
    ++task->pid;
    task->state = task_uninterruptible;
    Thread* thd = (Thread*)(task + 1);
    task->thread = thd;
    memcpy(regs, (void*)((unsigned long)task + STACK_SIZE - sizeof(pt_regs)), sizeof(pt_regs));
    thd->rsp0 = (unsigned long)task + STACK_SIZE;
    thd->rip = regs->rip;
    thd->rsp = thd->rsp0 - sizeof(pt_regs);
    if (!(task->flags & pf_kthread)) /* if is not a kernel process, process's entry is ret_from_intr */
        thd->rip = regs->rip = (unsigned long)ret_from_intr;

    task->state = task_running;
    return 0;
}

int kernel_thread(unsigned long (*fn)(unsigned long), unsigned long arg, unsigned long flags)
{
    pt_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rbx = (unsigned long)fn;
    regs.rdx = (unsigned long)arg;
    regs.ds = regs.es = regs.ss = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.rflags = (1 << 9);
    regs.rip = (unsigned long)kernel_thread_func;
    return do_fork(&regs, flags, 0, 0);
}

/* Must be invoked after sys_memory_init  */
extern unsigned long _stack_start;
void sys_task_init()
{
    load_TR(8);
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
    printk(KERN_INFO "g_init_mm is inited with pml4t %p, start_stack %p\n", g_init_mm.pml4t, to_pointer(g_init_mm.start_stack));
    /*
    set_tss64(&g_init_thread, g_init_tss[0].rsp0, g_init_tss[0].rsp1, g_init_tss[0].rsp2,
        g_init_tss[0].ist1, g_init_tss[0].ist2, g_init_tss[0].ist3, g_init_tss[0].ist4,
        g_init_tss[0].ist5, g_init_tss[0].ist6, g_init_tss[0].ist7);
    g_init_tss[0].rsp0 = g_init_thread.rsp0;
*/
    printk_tss(g_init_tss);

    list_init(&g_init_task_u.task.list);
    kernel_thread(init, 10, CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
    g_init_task_u.task.state = task_running;
    Task* current = get_current_task();
    printk(KERN_INFO "current task address is %p\n", current);
    Task* next = container_of(list_next(&current->list), Task, list);
    switch_to(current, next);
}