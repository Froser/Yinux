#pragma once
/* 类型信息 */
#include <stdbool.h>

#ifdef _MSC_VER
#define Yinux_Check_StaticSize(type, expected) \
    static_assert(sizeof(type) == expected, "size incorrect")
#else
#define Yinux_Check_StaticSize(type, expected)
#endif

#ifdef _MSC_VER
#  define YINUX_PACKED_STRUCT(name) \
    __pragma(pack(push, 1)) struct name
#  define YUNUX_PACKED_END() \
    __pragma(pack(pop))
#elif defined(__GNUC__)
#  define YINUX_PACKED_STRUCT(name) struct __attribute__((packed)) name
#  define YUNUX_PACKED_END()
#endif

typedef unsigned char DB;
Yinux_Check_StaticSize(DB, 1);

typedef unsigned short DW;
Yinux_Check_StaticSize(DW, 2);

typedef unsigned int DD;
Yinux_Check_StaticSize(DD, 4);

typedef unsigned long long DQ;
Yinux_Check_StaticSize(DQ, 8);

typedef unsigned short u16;
Yinux_Check_StaticSize(u16, 2);

typedef unsigned int u32;
Yinux_Check_StaticSize(u32, 4);