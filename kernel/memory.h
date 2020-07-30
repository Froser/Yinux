#pragma once
#include <yinux/kernel.h>
#include <yinux/list.h>

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
    unsigned long           total_pages_link;
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
    unsigned long start_data;
    unsigned long start_rodata;
    unsigned long end_code;
    unsigned long end_data;
    unsigned long end_rodata;
    unsigned long end_brk;

    unsigned long end_of_struct;
} Global_Memory_Descriptor;

typedef void* (*SlabStructor)(void* vaddr, unsigned long arg);
struct Slab_t;
typedef struct {
    unsigned long size;
    unsigned long total_using;
    unsigned long total_free;
    struct Slab_t* cache_pool;
    struct Slab_t* cache_dma_pool;
    SlabStructor constructor;
    SlabStructor destructor;
} Slab_cache;

typedef struct Slab_t {
    List list;
    Memory_Page* page;
    unsigned long using_count;
    unsigned long free_count;
    void* vaddr;
    unsigned long color_length;
    unsigned long color_count;
    unsigned long* color_map;
} Slab;

#define SIZEOF_LONG_ALIGN(s)    ((size + sizeof(long) - 1) & ~(sizeof(long) - 1))

#define PAGE_OFFSET     ((unsigned long)0xffff800000000000)

/* Virtual address to physics address */
#define MEM_V2P(addr)   ((unsigned long)(addr) - PAGE_OFFSET)

/* Physics address to virtual address */
#define MEM_P2V(addr)   ((unsigned long*)((unsigned long)(addr) + PAGE_OFFSET))

//  mapped=1 or un-mapped=0 
#define PG_PTable_Mapped (1 << 0)

//  init-code=1 or normal-code/data=0
#define PG_Kernel_Init  (1 << 1)

//  device=1 or memory=0
#define PG_Device   (1 << 2)

//  kernel=1 or user=0
#define PG_Kernel   (1 << 3)

//  shared=1 or single-use=0 
#define PG_Shared   (1 << 4)

#define ZONE_DMA_INDEX 0
#define ZONE_NORMAL_INDEX 0      //low 1GB RAM ,was mapped in pagetable
#define ZONE_UNMAPPED_INDEX 0    //above 1GB RAM,unmapped in pagetable

typedef enum {
    zone_dma = (1 << 0),
    zone_normal = (1 << 1),
    zone_unmapped = (1 << 2)
} zone_type ;

void sys_memory_init();
unsigned long* get_kernel_CR3();
const Global_Memory_Descriptor* get_kernel_memdesc();
Memory_Page* alloc_pages(int zone, int page_count, unsigned long page_flags);
void* kmalloc(unsigned long size, unsigned long gfp_flags);
bool kfree(void* address);