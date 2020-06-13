#pragma once
/* 类型信息 */

#define Yinux_Check_StaticSize(type, expected) \
	static_assert(sizeof(type) == expected, "size incorrect")

typedef unsigned char DB;
Yinux_Check_StaticSize(DB, 1);

typedef unsigned short DW;
Yinux_Check_StaticSize(DW, 2);

typedef unsigned int DD;
Yinux_Check_StaticSize(DD, 4);

typedef unsigned long long DQ;
Yinux_Check_StaticSize(DQ, 8);