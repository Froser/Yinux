#pragma once
#include <stdarg.h>
#include "types.h"

/* Compiler macros */
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x)   __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#ifndef __attribute_used__
#define __attribute_used__  __attribute__((__used__))
#endif

#if __GNUC__ == 3
#if __GNUC_MINOR__ >= 1
# define inline         __inline__ __attribute__((always_inline))
# define __inline__     __inline__ __attribute__((always_inline))
# define __inline       __inline__ __attribute__((always_inline))
#endif
#endif

/* no checker support, so we unconditionally define this as (null) */
#define __user
#define __iomem
/* End of compiler macros */

#define ALIGN_ADDRESS(addr, alignment) \
    (((unsigned long)(addr) + alignment - 1) & (~(alignment - 1)))

/* Pages */
#define PAGE_SHIFT      21      /* PDPTE.PS=0, PDE.PS=1, 2MB page size */
#define PAGE_SIZE       (1ul << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(a)   ALIGN_ADDRESS((a), PAGE_SIZE)

/* End of pages */

/* div 64 */
/*
 * do_div() is NOT a C function. It wants to return
 * two values (the quotient and the remainder), but
 * since that doesn't work very well in C, what it
 * does is:
 *
 * - modifies the 64-bit dividend _in_place_
 * - returns the 32-bit remainder
 *
 * This ends up being the most efficient "calling
 * convention" on x86.
 */
#define do_div(n,base) ({ \
    int __res; \
    __res = ((unsigned long) (n)) % (unsigned) (base); \
    (n) = ((unsigned long) (n)) / (unsigned) (base); \
    __res; })
/* End of div64 */

#define KERN_EMERG      "<0>"   /* system is unusable               */
#define KERN_ALERT      "<1>"   /* action must be taken immediately */
#define KERN_CRIT       "<2>"   /* critical conditions              */
#define KERN_ERR        "<3>"   /* error conditions                 */
#define KERN_WARNING    "<4>"   /* warning conditions               */
#define KERN_NOTICE     "<5>"   /* normal but significant condition */
#define KERN_INFO       "<6>"   /* informational                    */
#define KERN_DEBUG      "<7>"   /* debug-level messages             */

void init_printk();
int sprintf(char * buf, const char * fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
int vsprintf(char *buf, const char *, va_list)
    __attribute__ ((format (printf, 2, 0)));
int snprintf(char * buf, size_t size, const char * fmt, ...)
    __attribute__ ((format (printf, 3, 4)));
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
    __attribute__ ((format (printf, 3, 0)));
int printk(const char * fmt, ...)
    __attribute__ ((format (printf, 1, 2)));

#define sti() \
{ __asm__ __volatile__ ( "sti":::); }

#define io_out8(port, value) {                  \
    __asm__ __volatile__    (                   \
                    "outb   %0, %%dx    \n\t"   \
                    "mfence             \n\t"   \
                    ::"a"((byte)value),"d"(port)\
                    :"memory" );                \
}

#define dump_stack()            /* call in traps in future */

#define TAB_INDENTS     8

#define to_pointer(p) ((void*)(p))

/* Fast system call */
static unsigned long rdmsr(unsigned long address)
{
    unsigned int tmp0 = 0;
    unsigned int tmp1 = 0;
    __asm__ __volatile__ ("rdmsr \n\t":"=d"(tmp0),"=a"(tmp1):"c"(address):"memory");
    return (unsigned long)tmp0 << 32 | tmp1;
}

static void wrmsr(unsigned long address,unsigned long value)
{
    __asm__ __volatile__ ("wrmsr \n\t"::"d"(value >> 32),"a"(value & 0xffffffff),"c"(address):"memory"); 
}

/* (MSR address 174H) — The lower 16 bits of this MSR are the segment selector for the privilege level 0 code segment.
 This value is also used to determine the segment selector of the privilege level 0 stack segment (see the Operation section).
 This value cannot indicate a null selector. 
 */
#define IA32_SYSENTER_CS (0x174)

/*
 (MSR address 175H) — The value of this MSR is loaded into RSP (thus, this value contains the stack pointer for the privilege level 0 stack).
 This value cannot represent a non-canonical address.
 In protected mode, only bits 31:0 are loaded.
 */
#define IA32_SYSENTER_ESP (0x175)

/*
 (MSR address 176H) — The value of this MSR is loaded into RIP (thus, this value references the first instruction of the selected operating procedure or routine).
 In protected mode, only bits 31:0 are loaded.
 */
#define IA32_SYSENTER_EIP (0x176)
