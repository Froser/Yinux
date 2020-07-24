// GDT Table
.section .data.kernel
.globl GDT_Table

// Page
.align 8
.org 0x1000                              /* 0x101000 */
PML4E:
    .quad       0x102007
    .fill       255,8,0
    .quad       0x102007
    .fill       255,8,0

.org 0x2000
PDPTE:
    .quad       0x103003
    .fill       511,8,0

.org 0x3000
PDE:                                    /* PDE.PS=1, 2MB page size */
    .quad       0x000083
    .quad       0x200083
    .quad       0x400083
    .quad       0x600083
    .quad       0x800083
    .quad       0xe0000083
    .quad       0xe0200083
    .quad       0xe0400083
    .quad       0xe0600083
    .quad       0xe0800083
    .quad       0xe0a00083
    .quad       0xe0c00083
    .quad       0xe0e00083
    .fill       499,8,0

GDT_Table:
    .quad       0x0000000000000000      /* NULL */
    .quad       0x0020980000000000      /*1 KERNEL Code 64-bit Segment 08 */
    .quad       0x0020920000000000      /*2 KERNEL Data 64-bit Segment 10 */
    .quad       0x0000000000000000      /*3 USER Code 32-bit Segment 18 */
    .quad       0x0000000000000000      /*4 USER Data 32-bit Segment 20 */
    .quad       0x0020f80000000000      /*5 USER Code 64-bit Segment 28 */
    .quad       0x0020f20000000000      /*6 USER Data 64-bit Segment 30 */
    .quad       0x00cf9a000000ffff      /*7 KERNEL Code 32-bit Segment 38 */
    .quad       0x00cf92000000ffff      /*8 KERNEL Data 32-bit Segment 40 */
    .fill       10,8,0                  /*9~10 TSS 128-bit 40 */
GDT_END:

GDT_POINTER:
GDT_LIMIT:      .word   GDT_END - GDT_Table - 1
GDT_BASE:       .quad   GDT_Table

// IDT Table
.align 8                                /* For best performance */
.globl IDT_Table
IDT_Table:
    .fill   512,8,0
IDT_END:

IDT_POINTER:
IDT_LIMIT:      .word   IDT_END - IDT_Table - 1
IDT_BASE:       .quad   IDT_Table

// TSS64_Table
TSS64_Table:
    .fill   13,8,0
TSS64_END:

TSS64_POINTER:
TSS64_LIMIT:    .word   TSS64_END - TSS64_Table - 1
TSS64_BASE:     .quad   TSS64_Table

.org 0x6000

.section .text
.globl _start

_start:
    mov         $0x10,          %ax
    mov         $0x7e00,        %esp

// load gdt
    lgdt        GDT_POINTER(%rip)

// load idt
    lidt        IDT_POINTER(%rip)
    mov         $0x10,          %ax
    mov         %ax,            %gs
    movq        $0x7e00,        %rsp

// setup IDT
    leaq        default_int(%rip), %rdx
    movq        $(0x08 << 16),  %rax        /* Segment selector 8 */
    movw        %dx,            %ax
    movq        $(0x8e00 << 32), %rcx       /* IDT: P=1, DPL=0, S=0, Type=1110b (interrupt gate descriptor) */
    addq        %rcx,           %rax
    movl        %edx,           %ecx
    shrl        $16,            %ecx
    shlq        $48,            %rcx
    addq        %rcx,           %rax        /* interrupt function offset */
    shrq        $32,            %rdx
    leaq        IDT_Table(%rip), %rdi
    mov         $256,           %rcx        /* repeat times */
repeat_idt:
    movq        %rax,           (%rdi)
    movq        %rdx,           8(%rdi)
    addq        $0x10,          %rdi
    dec         %rcx
    jne         repeat_idt

// setup TSS64
    leaq        TSS64_Table(%rip), %rdx
    xorq        %rax,           %rax        /* TSS */
    xorq        %rcx,           %rcx
    movq        $0x89,          %rax
    shlq        $40,            %rax        /* TSS: P=1, B=0 */
    movl        %edx,           %ecx
    shrl        $24,            %ecx
    shlq        $56,            %rcx
    addq        %rcx,           %rax
    xorq        %rcx,           %rcx
    movl        %edx,           %ecx
    andl        $0xffffff,      %ecx
    shlq        $16,            %rcx
    addq        %rcx,           %rax        /* TSS: Segment base */
    addq        $103,           %rax
    leaq        GDT_Table(%rip), %rdi
    movq        %rax,           80(%rdi)    /* Write TSS to GDT (at 10) */
    shrq        $32,            %rdx
    movq        %rdx,           88(%rdi)

// load cr3 (Page directory base)
    movq        $0x101007,      %rax
    movq        %rax,           %cr3
    movq        fake_call(%rip), %rax
    pushq       $0x08
    pushq       %rax
    lretq

fake_call:
    .quad       entry64

entry64:
// call kernel
    mov         $0x10,          %ax
    mov         %ax,            %gs
    movq        _stack_start(%rip), %rsp   /* assign rsp to kernel stack start (32KB) */
    movq        go_kernel(%rip), %rax
    pushq       $0x08
    pushq       %rax
    lretq

go_kernel:
    .quad       Kernel_Main

default_int:
    call        int_failure     /* stuck here */