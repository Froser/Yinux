#define __ASSEMBLY__
#include "../include/yinux/linkage.h"

#define T_ENTRY_NO_ERRCODE(entry)         \
ENTRY(entry)                                \
    pushq   $0;                             \
    pushq   %rax;                           \
    leaq    do_##entry##(%rip),     %rax;   \
    xchgq   %rax,   (%rsp);                 \
    jmp     error_code;

#define T_ENTRY(entry)                      \
ENTRY(entry)                                \
    pushq   %rax;                           \
    leaq    do_##entry##(%rip),       %rax; \
    xchgq   %rax,   (%rsp);                 \
    jmp     error_code;

ENTRY(_stack_start)
    .quad g_init_task_u + 32768

/* Stack layout */
R15 = 0x00
R14 = 0x08
R13 = 0x10
R12 = 0x18
R11 = 0x20
R10 = 0x28
R9 = 0x30
R8 = 0x38
RBX = 0x40
RCX = 0x48
RDX = 0x50
RSI = 0x58
RDI = 0x60
RBP = 0x68
DS = 0x70
ES = 0x78
RAX = 0x08
FUNC = 0x88
ERRCODE = 0x90
RIP = 0x98
CS = 0xa0
RFLAGS = 0xa8
OLDRSP = 0xb0
OLDSS = 0xb8

RESTORE_ALL:
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %r11
    popq    %r10
    popq    %r9
    popq    %r8
    popq    %rbx
    popq    %rcx
    popq    %rdx
    popq    %rsi
    popq    %rdi
    popq    %rbp
    popq    %rax
    movq    %rax,   %ds   /* pop ds */
    popq    %rax
    movq    %rax,   %es   /* pop es */
    addq    $0x10,  %rsp  /* pop FUNC and ERRCODE */
    iretq

ENTRY(nmi)
    pushq   %rax
    cld
    movq    %es,    %rax
    pushq   %rax          /* push es */
    movq    %ds,    %rax
    pushq   %rax          /* push ds */
    pushq   %rbp
    pushq   %rdi
    pushq   %rsi
    pushq   %rdx
    pushq   %rcx
    pushq   %rbx
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    movq    $0x10,  %rdx
    movq    %rdx,   %ds
    movq    %rdx,   %es
    movq    $0,     %rsi
    movq    %rsp,   %rdi
    callq   do_nmi
    jmp     RESTORE_ALL

T_ENTRY_NO_ERRCODE(divide_error)
T_ENTRY_NO_ERRCODE(debug)
T_ENTRY_NO_ERRCODE(breakpoint)
T_ENTRY_NO_ERRCODE(overflow)
T_ENTRY_NO_ERRCODE(bound_range_excceeded);
T_ENTRY_NO_ERRCODE(invalid_opcode);
T_ENTRY_NO_ERRCODE(device_not_available);
T_ENTRY(double_fault);
T_ENTRY(invalid_tss);
T_ENTRY(segment_not_present);
T_ENTRY(stack_segment_fault);
T_ENTRY(general_protection);
T_ENTRY(page_fault)
T_ENTRY_NO_ERRCODE(x87_floating_point_exception);
T_ENTRY(alignment_check);
T_ENTRY_NO_ERRCODE(machine_check);

error_code:
    pushq   %rax
    movq    %es,    %rax
    pushq   %rax          /* push es */
    movq    %ds,    %rax
    pushq   %rax          /* push ds */
    xorq    %rax,   %rax
    pushq   %rbp
    pushq   %rdi
    pushq   %rsi
    pushq   %rdx
    pushq   %rcx
    pushq   %rbx
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    cld

    movq    ERRCODE(%rsp),  %rsi    /* 2nd parameter */
    movq    FUNC(%rsp),     %rdx
    movq    $0x10,  %rdi
    movq    %rdi,   %ds
    movq    %rdi,   %es
    movq    %rsp,   %rdi            /* 1st parameter */
    callq   *%rdx

    jmp     ret_from_exception

ret_from_exception:

ENTRY(ret_from_intr)
    jmp     RESTORE_ALL