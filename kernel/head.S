// GDT Table
.section .data
.globl GDT_Table

GDT_Table:
    .quad       0x0000000000000000      /* NULL */
    .quad       0x0020980000000000      /*1 KERNEL Code 64-bit Segment 08 */
    .quad       0x0020920000000000      /*2 KERNEL Data 64-bit Segment 10 */
    .quad       0x0020f80000000000      /*3 USER Code 64-bit Segment 18 */
    .quad       0x0020f20000000000      /*4 USER Data 64-bit Segment 20 */
    .quad       0x00cf9a000000ffff      /*5 KERNEL Code 32-bit Segment 28 */
    .quad       0x00cf92000000ffff      /*6 KERNEL Data 32-bit Segment 30 */
    .fill       10,8,0                  /*8~9 TSS 128-bit 40 */
GDT_END:

GDT_POINTER:
GDT_LIMIT:      .word   GDT_END - GDT_Table - 1
GDT_BASE:       .quad   GDT_Table

// IDT Table
.align 8                                /* For best performance */
.globl IDT_Table
IDT_Table:
    .fill   256,8,0
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

// Page
.align 8
.org 0x1000
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
PDE:
    .quad       0x000083
    .quad       0x200083
    .quad       0x400083
    .quad       0x600083
    .quad       0x800083
    .quad       0xe000083
    .quad       0xe020083
    .quad       0xe040083
    .quad       0xe060083
    .quad       0xe080083
    .quad       0xe0a0083
    .quad       0xe0c0083
    .quad       0xe0e0083
    .fill       499,8,0


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

// load cr3 (Page directory base)
    movq        $0x101080,      %rax
    movq        %rax,           %cr3
    movq        fake_call(%rip),%rax
    pushq       $0x08
    pushq       %rax
    lretq

fake_call:
    .quad       entry64

entry64:
    mov         $0x10,          %ax
    mov         %ax,            %gs
    movq        $0xffff800000007e00, %rsp   /* restore rsp */
    movq        go_kernel(%rip),%rax
    pushq       $0x08
    pushq       %rax
    lretq

go_kernel:
    .quad       Kernel_Main
