#pragma once
#include <yinux/types.h>

YINUX_PACKED_STRUCT(gate_struct64) {
    u16 offset_low;
    u16 segment;
    unsigned ist : 3, zero0 : 5, type: 5, dpl: 2, p: 1;
    u16 offset_middle;
    u32 offset_high;
    u32 zero1;
};
YUNUX_PACKED_END()