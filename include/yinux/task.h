#pragma once
#include <yinux/list.h>

#define KERNEL_CS  (0x08)
#define KERNEL_DS  (0x10)
#define USER_CS    (0x28)
#define USER_DS    (0x30)
#define STACK_SIZE (32 * 1024)

typedef struct {
    unsigned long* pml4t;
    unsigned long start_code, end_code;
    unsigned long start_data, end_data;
    unsigned long start_rodata, end_rodata;
    unsigned long start_brk, end_brk;
    unsigned long start_stack;
} mm_struct;

typedef struct Thread_t {
    unsigned long rsp0;
    unsigned long rip;
    unsigned long rsp;
    unsigned long fs;
    unsigned long gs;
    unsigned long cr2;
    unsigned long trap_nr;
    unsigned long error_code;
} Thread;

typedef struct Task_t {
    List list;
    volatile long state;
    unsigned long flags;
    mm_struct* mm;
    struct Thread_t* thread;
    unsigned long addr_limit; /* 0x0000000000000000 - 0x00007fffffffffff user */
                              /* 0xffff800000000000 - 0xffffffffffffffff kernel */
    long pid;
    long counter;
    long signal;
    long priority;
} Task;

typedef union {
    struct Task_t task;
    unsigned long stack[STACK_SIZE / sizeof(unsigned long)];
} __attribute__((aligned(8))) task_u;

typedef struct {
    unsigned int reserved0;
    unsigned long rsp0;
    unsigned long rsp1;
    unsigned long rsp2;
    unsigned long reserved1;
    unsigned long ist1;
    unsigned long ist2;
    unsigned long ist3;
    unsigned long ist4;
    unsigned long ist5;
    unsigned long ist6;
    unsigned long ist7;
    unsigned long reserved2;
    unsigned short reserved3;
    unsigned short iomapbaseaddr;
} __attribute__((packed)) TSS;

typedef enum {
    task_running = (1 << 0),
    task_interruptible = (1 << 1),
    task_uninterruptible = (1 << 2),
    task_zombie = (1 << 3),
    task_stopped = (1 << 4)
} Task_state;

typedef enum {
    pf_kthread = 1ul << 0,
    pf_need_schedule = 1ul << 1,
    pf_vfork = 1ul << 2,
} Task_flag;

Task* get_current_task();
void sys_task_init();