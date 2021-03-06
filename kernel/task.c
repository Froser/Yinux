#include <yinux/task.h>
#include <yinux/cpu.h>
#include <yinux/trap.h>
#include <yinux/memory.h>
#include <yinux/ptrace.h>
#include <yinux/string.h>
#include <yinux/list.h>
#include <yinux/sched.h>

#define load_TR(n)                      \
do {                                    \
    __asm__ __volatile__( "ltr  %%ax"   \
        :                               \
        :"a"(n << 3)                    \
        :"memory");                     \
} while(0)

#define INIT_RSP0 ((unsigned long)(g_init_task_u.stack + STACK_SIZE / sizeof(unsigned long)))

#define INIT_TASK(tsk)                  \
{                                       \
    .state = task_uninterruptible,      \
    .flags = pf_kthread,                \
    .mm = &g_init_mm,                   \
    .thread = &g_init_thread,           \
    .addr_limit = 0xffff800000000000,   \
    .pid = 0,                           \
    .counter = 1,                       \
    .signal = 0,                        \
    .priority = 0                       \
}

#define INIT_TSS                        \
{                                       \
    .reserved0 = 0,                     \
    .rsp0 = INIT_RSP0,                  \
    .rsp1 = INIT_RSP0,                  \
    .rsp2 = INIT_RSP0,                  \
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
    printk(KERN_INFO "TSS of 0x%p details:\nrsp0=0x%p\nrsp1=0x%p\nrsp2=0x%p\nist1=0x%p\nist2=0x%p\nist3=0x%p\nist4=0x%p\nist5=0x%p\nist6=0x%p\nist7=0x%p\n",
        tss,
        to_pointer(tss->rsp0), to_pointer(tss->rsp1), to_pointer(tss->rsp2),
        to_pointer(tss->ist1), to_pointer(tss->ist2), to_pointer(tss->ist3),
        to_pointer(tss->ist4), to_pointer(tss->ist5), to_pointer(tss->ist6), to_pointer(tss->ist7)
        );
}

void printk_task(const Task* task) {
    printk(KERN_INFO "The task in %p details:\npid: %ld\nthread.rsp0=%#018lx\nthread.rip=%#018lx\n"
        "thread.rsp=%#018lx\nthread.fs=%#018lx\nthread.gs=%#018lx\nthread.cr2=%#018lx\n"
        "thread.trap_nr=%#018lx\nthread.error_code=%#018lx\n",
        task, task->pid, task->thread->rsp0, task->thread->rip, task->thread->rsp, task->thread->fs,
        task->thread->gs, task->thread->cr2, task->thread->trap_nr, task->thread->error_code);
}

/* Init task and stack space union, thread and tss */
extern Thread g_init_thread;
task_u g_init_task_u __attribute__((__section__ (".data.init_task"))) = {
    INIT_TASK(g_init_task_u.task)
};
Task* g_init_task[NR_CPUS] = { &g_init_task_u.task };
TSS g_init_tss[NR_CPUS] = { [0 ... NR_CPUS - 1] = INIT_TSS };
Thread g_init_thread = {
    .rsp0 = INIT_RSP0,
    .rsp = INIT_RSP0,
    .fs = KERNEL_DS,
    .gs = KERNEL_DS,
    .cr2 = 0,
    .trap_nr = 0,
    .error_code = 0
};

Task* get_current_task()
{
    /* Task struct is 32KB aligned. So we can just move %rsp to its alignment to get the task union. */
    /* Task struct in task union shares the same address of stack_start. */
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

/* RDI = prev, RSI = next. 
  next thread's rip has been put before jump to this function. 
  So the task will be switched after this return.
  */
void __switch_to(Task* prev, Task* next)
{
    printk(KERN_INFO "prev task(0x%p) thread rip: %#018lx, rsp: %#018lx\nnext task(0x%p) thread rip: %#018lx, rsp: %#018lx\n",
        prev, prev->thread->rip, prev->thread->rsp,
        next, next->thread->rip, next->thread->rsp);

    g_init_tss[0].rsp0 = next->thread->rsp0;
    __asm__ __volatile__ ("movq %%fs, %0 \n\t":"=a"(prev->thread->fs));
    __asm__ __volatile__ ("movq %%gs, %0 \n\t":"=a"(prev->thread->gs));
    __asm__ __volatile__ ("movq %0, %%gs \n\t"::"a"(next->thread->fs));
    __asm__ __volatile__ ("movq %0, %%fs \n\t"::"a"(next->thread->gs));
}

unsigned long do_exit(unsigned long code)
{
    printk("task exited with result code %lu", code);
    while(1);
}

/* syscall */
unsigned long no_system_call(PTrace_regs* regs)
{
    printk(KERN_INFO "System call not handled with rax %ld\n", regs->rax);
    return 2;
}

#define MAX_SYSCALL_NR 128
typedef unsigned long (*system_call_t)(PTrace_regs* regs);
system_call_t g_system_call_table[MAX_SYSCALL_NR] = {
    [0 ... MAX_SYSCALL_NR - 1] = no_system_call
};

unsigned long system_call_function(PTrace_regs* regs)
{
    return g_system_call_table[regs->rax](regs);
}

/* Create process */
void ret_from_intr();
void kernel_thread_func(void);
void ret_system_call(void);
void system_call(void);

void test_user_code()
{
    long ret;
    /* store return address and current rsp for sysexit */
    __asm__ __volatile__ ( "leaq sysexit_return_address(%%rip), %%rdx \n\t"
                           "movq %%rsp, %%rcx                         \n\t"
                           "sysenter                                  \n\t"
                           "sysexit_return_address:                   \n\t"
                           :"=a"(ret):"0"(15):"memory"
                         );
    while (1);
}
void test_user_code_end() {}

void do_execve(PTrace_regs* regs)
{
    regs->rdx = 0x800000; /* ring3 rip */
    regs->rcx = 0xa00000; /* ring3 rsp */
    regs->ds = regs->es = 0;
    memcpy(to_pointer(regs->rdx), test_user_code, test_user_code_end - test_user_code);
    printk(KERN_INFO "do_execve called.\n");
}

unsigned long init(unsigned long arg)
{
    Task* current = get_current_task();

    /* prepare system call to ring3 */
    current->thread->rip = (unsigned long)ret_system_call;
    PTrace_regs* regs = (PTrace_regs*)current->thread->rsp;

    /* push current thread's rip, and set rsp. rdx = regs */
    __asm__ __volatile__ ( "movq %1, %%rsp \n\t"
                           "pushq %2       \n\t"
                           "jmp do_execve  \n\t"
                           ::"D"(regs), "m"(current->thread->rsp),"m"(current->thread->rip)
                           :"memory"
                        );

    return 1;
}

unsigned long do_fork(PTrace_regs* regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size)
{
    /* allocate a new page to construct a Task. */
    Memory_Page* p = alloc_pages(zone_normal, 1, PG_PTable_Mapped | PG_Kernel);
    Task* task = (Task*)MEM_P2V(p->phy_addr);
    memset(task, 0, sizeof(*task));
    *task = *get_current_task();
    printk(KERN_INFO "new task address is %p\n", task);

    list_init(&task->list);
    list_push_front(&g_init_task_u.task.list, &task->list); /* init -> task */
    ++task->pid;
    task->state = task_uninterruptible;

    /* put the Thread next to the task */
    Thread* thd = (Thread*)(task + 1);
    task->thread = thd;

    /* rsp0 is the base pointer of the stack (highest) */
    thd->rsp0 = (unsigned long)task + STACK_SIZE;
    thd->rip = regs->rip;
    thd->rsp = thd->rsp0 - sizeof(PTrace_regs);
    printk("task flags is %ld\n", task->flags);
    if (!(task->flags & pf_kthread)) /* if is not a kernel process, process's entry is ret_system_call */
        thd->rip = regs->rip = (unsigned long)ret_system_call;

    /* put PTrace_regs at the top of the process stack */
    memcpy((void*)((unsigned long)task + STACK_SIZE - sizeof(PTrace_regs)), regs, sizeof(PTrace_regs));

    task->state = task_running;
    printk_task(task);
    return 0;
}

int kernel_thread(unsigned long (*fn)(unsigned long), unsigned long arg, unsigned long flags)
{
    PTrace_regs regs;
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
    load_TR(10);

    /* set addresses of ring3 to ring0 */
    wrmsr(IA32_SYSENTER_CS, KERNEL_CS);
    wrmsr(IA32_SYSENTER_ESP, get_current_task()->thread->rsp0);
    wrmsr(IA32_SYSENTER_EIP, (unsigned long)system_call);

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
    printk(KERN_INFO "init task address is 0x%p, and its stack address starts at 0x%p, size is %#018lx\n",
        &g_init_task_u.task, &g_init_task_u.stack, sizeof(g_init_task_u.stack));
    printk(KERN_INFO "g_init_mm is inited with pml4t 0x%p, start_stack 0x%p\n", g_init_mm.pml4t, to_pointer(g_init_mm.start_stack));
    printk_tss(g_init_tss);

    list_init(&g_init_task_u.task.list);
    kernel_thread(init, 10, CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
    g_init_task_u.task.state = task_running;
    Task* current = get_current_task();
    Task* next = container_of(list_next(&current->list), Task, list);
    switch_to(current, next);
}