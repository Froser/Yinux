#pragma once
#include <yinux/kernel.h>

struct Memory_Page_t;
typedef struct {
    struct Memory_Page_t*   pages_group;
    unsigned long           pages_length;
    unsigned long           zone_start_addr;
    unsigned long           zone_end_addr;
    unsigned long           zone_length;
    unsigned long           attribute;
    unsigned long           page_using_cnt;
    unsigned long           page_free_cnt;
    unsigned long           page_pages_link;
} Memory_Zone;

typedef struct Memory_Page_t {
    Memory_Zone*            zones_struct;
    unsigned long           phy_addr;
    unsigned long           attribute;
    unsigned long           ref_cnt;
    unsigned long           age;
} Memory_Page;

typedef struct {
    unsigned int address1;
    unsigned int address2;
    unsigned int length1;
    unsigned int length2;
    unsigned int type;
} Memory_E820_32;
Yinux_Check_StaticSize(Memory_E820_32, 20);

typedef struct {
    unsigned long address;
    unsigned long length;
    unsigned int type;
} __attribute((packed)) Memory_E820_64;
Yinux_Check_StaticSize(Memory_E820_64, 20);

typedef union {
    Memory_E820_32 e820_32;
    Memory_E820_64 e820_64;
} Memory_E820;
Yinux_Check_StaticSize(Memory_E820, 20);

#define MEM_MAX 32
typedef struct {
    Memory_E820 e820[MEM_MAX];
    unsigned long e820_len;

    /* bitmap */
    unsigned long* bits_map;
    unsigned long bits_size;
    unsigned long bits_length;

    /* pages */
    Memory_Page*  pages_struct;
    unsigned long pages_size;
    unsigned long pages_length;

    /* zones */
    Memory_Zone*  zones_struct;
    unsigned long zones_size;
    unsigned long zones_length;

    /* sections */
    unsigned long start_code;
    unsigned long end_code;
    unsigned long end_data;
    unsigned long end_brk;
} Global_Memory_Descriptor;

void init_memory();