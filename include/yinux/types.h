#pragma once
/* 类型信息 */
#include <stdbool.h>

#ifdef _MSC_VER
#define Yinux_Check_StaticSize(type, expected) \
    static_assert(sizeof(type) == expected, "size incorrect")
#else
#define Yinux_Check_StaticSize(type, expected)
#endif

typedef unsigned char DB;
Yinux_Check_StaticSize(DB, 1);

typedef unsigned short DW;
Yinux_Check_StaticSize(DW, 2);

typedef unsigned int DD;
Yinux_Check_StaticSize(DD, 4);

typedef unsigned long long DQ;
Yinux_Check_StaticSize(DQ, 8);

typedef unsigned char byte;
Yinux_Check_StaticSize(byte, 1);

typedef unsigned short u16;
Yinux_Check_StaticSize(u16, 2);

typedef unsigned int u32;
Yinux_Check_StaticSize(u32, 4);

/* Basic types */
#ifndef NULL
#define NULL 0
#endif

typedef unsigned long   __kernel_dev_t;
typedef unsigned long   __kernel_ino_t;
typedef unsigned int    __kernel_mode_t;
typedef unsigned long   __kernel_nlink_t;
typedef long            __kernel_off_t;
typedef int             __kernel_pid_t;
typedef int             __kernel_ipc_pid_t;
typedef unsigned int    __kernel_uid_t;
typedef unsigned int    __kernel_gid_t;
typedef unsigned long   __kernel_size_t;
typedef long            __kernel_ssize_t;
typedef long            __kernel_ptrdiff_t;
typedef long            __kernel_time_t;
typedef long            __kernel_suseconds_t;
typedef long            __kernel_clock_t;
typedef int             __kernel_daddr_t;
typedef char *          __kernel_caddr_t;
typedef unsigned short  __kernel_uid16_t;
typedef unsigned short  __kernel_gid16_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef __kernel_size_t     size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef __kernel_ssize_t    ssize_t;
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef __kernel_ptrdiff_t  ptrdiff_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef __kernel_time_t     time_t;
#endif

#ifndef _CLOCK_T
#define _CLOCK_T
typedef __kernel_clock_t    clock_t;
#endif

#ifndef _CADDR_T
#define _CADDR_T
typedef __kernel_caddr_t    caddr_t;
#endif

/* bsd */
typedef unsigned char       u_char;
typedef unsigned short      u_short;
typedef unsigned int        u_int;
typedef unsigned long       u_long;

/* sysv */
typedef unsigned char       unchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;