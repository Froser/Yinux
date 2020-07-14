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

/* Pages */

#define PAGE_SHIFT      22      /* PDPTE.PS=0, PDE.PS=1, 2MB page size */
#define PAGE_SIZE       1u << PAGE_SHIFT

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

#define dump_stack()            /* call in traps in future */

#define TAB_INDENTS     8