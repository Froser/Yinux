#include "memory.h"
#include <yinux/string.h>

#define PAGE_SHIFT_4K   12      /* PDPTE.PS=0, PDE.PS=1, 2MB page size */
#define PAGE_SIZE_4K    (1ul << PAGE_SHIFT_4K)

static Global_Memory_Descriptor g_mem_descriptor;
static unsigned long* g_pml4e;

unsigned long* g_kernelCR3;

typedef enum {
    MT_Available = 1,
    MT_Reserved_Invalid,
    MT_ACPI,
    MT_ACPINVS
} Memory_Type;

unsigned long* get_kernel_CR3()
{
    return g_kernelCR3;
}

const Global_Memory_Descriptor* get_kernel_memdesc()
{
    return &g_mem_descriptor;
}

void flush_tlb()
{
    unsigned long t;
    __asm__ __volatile__ (
        "movq %%cr3, %0     \n\t"
        "movq %0,    %%cr3  \n\t"
        :"=r"(t)::"memory"
        );
}

unsigned long* get_pml4e()
{
    unsigned long* t;
    __asm__ __volatile__ (
        "movq %%cr3, %0    \n\t"
        :"=r"(t)::"memory"
        );
    return t;
}

static const char* memtypestr(unsigned int type)
{
    switch (type) {
        case MT_Available:
            return "Available";
        case MT_Reserved_Invalid:
            return "Reserved or invalid";
        case MT_ACPI:
            return "ACPI";
        case MT_ACPINVS:
            return "ACPINVS";
        default:
            return "Unknown";
    }
}

Slab_cache kmalloc_cache_size[16] = 
{
    {32 ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {64 ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {128    ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {256    ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {512    ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {1024   ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},         //1KB
    {2048   ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {4096   ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},         //4KB
    {8192   ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {16384  ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {32768  ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {65536  ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},         //64KB
    {131072 ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},         //128KB
    {262144 ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {524288 ,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},
    {1048576,0  ,0  ,NULL   ,NULL   ,NULL   ,NULL},         //1MB
};

extern char _text;
extern char _etext;
extern char _data;
extern char _edata;
extern char _end;
extern char _rodata;
extern char _erodata;

bool page_init(Memory_Page* page, unsigned long flags)
{
    const unsigned long pagecnt = page->phy_addr >> PAGE_SHIFT;
    if (!page->attribute) {
        *(g_mem_descriptor.bits_map + (pagecnt >> 6)) |= 1ul << (pagecnt) % 64;
        page->attribute = flags;
        ++page->ref_cnt;
        ++page->zones_struct->page_using_cnt;
        --page->zones_struct->page_free_cnt;
        ++page->zones_struct->total_pages_link;
    /* } else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U)) {
        page->attribute |= flags;
        ++page->ref_cnt;
        ++page->zones_struct->total_pages_link;
    */
    } else {
        *(g_mem_descriptor.bits_map + (pagecnt >> 6)) |= 1ul << (pagecnt) % 64;
        page->attribute |= flags;
    }
    return true;
}

void sys_memory_init()
{
    /* see also Kernel.lds */
    g_mem_descriptor.start_code = (unsigned long)&_text;
    g_mem_descriptor.start_data = (unsigned long)&_data;
    g_mem_descriptor.end_code = (unsigned long)&_etext;
    g_mem_descriptor.end_data = (unsigned long)&_edata;
    g_mem_descriptor.start_rodata = (unsigned long)&_rodata;
    g_mem_descriptor.end_rodata = (unsigned long)&_erodata;
    g_mem_descriptor.end_brk = (unsigned long)&_end;

#if KERNEL_VERBOSE
    printk(KERN_INFO "Section layouts:\n.code: %#018lx-%#018lx\nend of .data:%#018lx\nbrk:%#018lx\n",
        g_mem_descriptor.start_code, g_mem_descriptor.end_code, g_mem_descriptor.end_data, g_mem_descriptor.end_brk);
#endif

    int memcount = *((int*)0xffff800000007e00); /* see also: boot/loader.asm Get_Mem_Info */
    Memory_E820* mem = (Memory_E820*)0xffff800000007e04;
    unsigned long availabletotal = 0;
    unsigned long pagetotal = 0;
    g_mem_descriptor.e820_len = memcount;

#if KERNEL_VERBOSE
    printk(KERN_INFO "Memory info blocks (%d):\n", memcount);
#endif

    for (int i = 0; i < memcount; ++i) {
        Memory_E820* m = mem + i;
        g_mem_descriptor.e820[i] = *m;

        #if KERNEL_VERBOSE
        printk(KERN_INFO "Address: %#010x,%08x\tLength:%#010x,%08x\tType:%s\n", 
            m->e820_32.address2, m->e820_32.address1, m->e820_32.length2, m->e820_32.length1, memtypestr(m->e820_32.type));
        #endif

        if (m->e820_64.type == MT_Available) {
            unsigned long start, end;
            availabletotal += m->e820_64.length;
            start = PAGE_ALIGN(m->e820_64.address); /* calculate align */
            end = ((m->e820_64.address + m->e820_64.length) >> PAGE_SHIFT) << PAGE_SHIFT;
            if (end > start)
                pagetotal += (end - start) >> PAGE_SHIFT;
        }
    }

    /* Assuming that the last e820 struct is the largest address */
    unsigned long memtotal = g_mem_descriptor.e820[memcount - 1].e820_64.address +  g_mem_descriptor.e820[memcount - 1].e820_64.length;
    printk(KERN_INFO "%lu (%#018lx) memory bytes (%ld MBs) detected.\n", memtotal, memtotal, (memtotal >> 20));
    printk(KERN_INFO "%lu (%#018lx) available bytes (%ld MBs) detected.\n", availabletotal, availabletotal, (availabletotal >> 20));
    printk(KERN_INFO "%lu (%#018lx) available pages detected.\n", pagetotal, pagetotal);

    /* allocate bits map */
    g_mem_descriptor.bits_map = (unsigned long*) ALIGN_ADDRESS(g_mem_descriptor.end_brk, PAGE_SIZE_4K); /* align 4KB */
    g_mem_descriptor.bits_size = memtotal >> PAGE_SHIFT; /* total pages */
    g_mem_descriptor.bits_length = (((unsigned long)(g_mem_descriptor.bits_size) + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
    memset(g_mem_descriptor.bits_map, 0xff, g_mem_descriptor.bits_length);

    /* allocate page structs */
    g_mem_descriptor.pages_struct = (Memory_Page*) ALIGN_ADDRESS(g_mem_descriptor.bits_map + g_mem_descriptor.bits_length, PAGE_SIZE_4K);
    g_mem_descriptor.pages_size = g_mem_descriptor.bits_size;
    g_mem_descriptor.pages_length = (g_mem_descriptor.pages_size * sizeof(Memory_Page) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    memset(g_mem_descriptor.pages_struct, 0x00, g_mem_descriptor.pages_length);

    /* allocate zones structs */
    g_mem_descriptor.zones_struct = (Memory_Zone*) ALIGN_ADDRESS(g_mem_descriptor.pages_struct + g_mem_descriptor.pages_length, PAGE_SIZE_4K);
    g_mem_descriptor.zones_size = 0;
    g_mem_descriptor.zones_length = ALIGN_ADDRESS(5 * sizeof(Memory_Zone), sizeof(long));
    memset(g_mem_descriptor.zones_struct, 0x00, g_mem_descriptor.zones_length);

    /* setup zones, pages and bits map */
    for (int i = 0; i < memcount; ++i) {
        unsigned long start, end;
        Memory_Zone* z;
        Memory_Page* p;
        unsigned long* b; /* bits */
        Memory_E820* m = g_mem_descriptor.e820 + i;
        if (m->e820_64.type == MT_Available) {
            start = PAGE_ALIGN(m->e820_64.address); /* calculate align */
            end = ((m->e820_64.address + m->e820_64.length) >> PAGE_SHIFT) << PAGE_SHIFT;
            if (end > start) {
                z = g_mem_descriptor.zones_struct + g_mem_descriptor.zones_size;
                ++g_mem_descriptor.zones_size;
                z->zone_start_addr = start;
                z->zone_end_addr = end;
                z->zone_length = end - start;
                z->page_using_cnt = 0;
                z->page_free_cnt = (end - start) >> PAGE_SHIFT;
                z->total_pages_link = 0;
                z->attribute = 0;
                z->pages_length = z->page_free_cnt;
                z->pages_group = (Memory_Page*)(g_mem_descriptor.pages_struct + (start >> PAGE_SHIFT));
                p = z->pages_group;
                for (int j = 0; j < z->pages_length; ++j, ++p) {
                    p->zones_struct = z;
                    p->phy_addr = start + PAGE_SIZE * j;
                    p->attribute = 0;
                    p->ref_cnt = 0;
                    p->age = 0;
                    *(g_mem_descriptor.bits_map + ((p->phy_addr >> PAGE_SHIFT) >> 6)) ^= 1ul << (p->phy_addr >> PAGE_SHIFT) % 64;
                }
            }
        }
    }

    /* we cannot use physics address below 2MB */
    g_mem_descriptor.pages_struct->zones_struct = g_mem_descriptor.zones_struct;
    g_mem_descriptor.pages_struct->phy_addr = 0ul;
    g_mem_descriptor.pages_struct->attribute = 0;
    g_mem_descriptor.pages_struct->ref_cnt = 0;
    g_mem_descriptor.pages_struct->age = 0;

    g_mem_descriptor.end_of_struct = (unsigned long)((unsigned long) g_mem_descriptor.zones_struct + g_mem_descriptor.zones_length 
        + sizeof(long) * 32) & (~(sizeof(long)) - 1); /* Reserved few bytes */

    printk(KERN_INFO "Memory descriptor address is %p\n", &g_mem_descriptor);
    printk(KERN_INFO "Memory descriptor's bits map start at %p, size is %#018lx, length is %#018lx\n",
        g_mem_descriptor.bits_map, g_mem_descriptor.bits_size, g_mem_descriptor.bits_length);
    printk(KERN_INFO "Memory descriptor's pages structs start at %p, size is %#018lx, length is %#018lx\n",
        g_mem_descriptor.pages_struct, g_mem_descriptor.pages_size, g_mem_descriptor.pages_length);
    printk(KERN_INFO "Memory descriptor's zones structs start at %p, size is %#018lx, length is %#018lx\n",
        g_mem_descriptor.zones_struct, g_mem_descriptor.zones_size, g_mem_descriptor.zones_length);

#if KERNEL_VERBOSE
    printk(KERN_INFO "Zone details: \n");
    for (int i = 0; i < g_mem_descriptor.zones_size; ++i) {
        Memory_Zone* zone = g_mem_descriptor.zones_struct + i;
        printk(KERN_INFO "Zone start address: %#018lx, end address: %#018lx, length: %#018lx\n", zone->zone_start_addr, zone->zone_end_addr, zone->zone_length);
    }
    printk(KERN_INFO "Memory structs end at %#018lx\n", g_mem_descriptor.end_of_struct);
#endif

    g_kernelCR3 = get_pml4e();
    int i = MEM_V2P(g_mem_descriptor.end_of_struct) >> PAGE_SHIFT;
    for (int j = 0; j <= i; ++j) {
        page_init(g_mem_descriptor.pages_struct + j, PG_PTable_Mapped | PG_Kernel_Init | PG_Kernel);
    }

    //flush_tlb();
}

Memory_Page* alloc_pages(int zone, int page_count, unsigned long page_flags)
{
    int zone_start = 0, zone_end = 0;
    unsigned long page = 0;
    switch (zone) {
        case zone_dma:
            zone_start = 0;
            zone_end = ZONE_DMA_INDEX;
            break;
        case zone_normal:
            zone_start = ZONE_DMA_INDEX;
            zone_end = ZONE_NORMAL_INDEX;
            break;
        case zone_unmapped:
            zone_start = ZONE_UNMAPPED_INDEX;
            zone_end = g_mem_descriptor.zones_size - 1;
            break;
        default:
            printk("alloc_pages error with unknown zone type.\n");
            return NULL;
    }

    for (int i = zone_start; i <= zone_end; ++i) {
        Memory_Zone* z = g_mem_descriptor.zones_struct + i;
        if (z->page_free_cnt < page_count) /* not enough page */
            continue;
        unsigned long start = z->zone_start_addr >> PAGE_SHIFT;
        unsigned long end = z->zone_end_addr >> PAGE_SHIFT;
        unsigned long lenght = z->zone_length >> PAGE_SHIFT;
        unsigned long t = 64 - start % 64;
        for (int j = start; j <= end; j += (j % 64 ? t : 64)) {
            unsigned long* p = g_mem_descriptor.bits_map + (j >> 6);
            unsigned long shift = j % 64;
            for (unsigned long k = shift; k < 64 - shift; ++k) {
                if (!(((*p >> k) | (*(p + 1) << (64 - k))) & (page_count == 64 ? 0xffffffffffffffffUL : ((1UL << page_count) - 1)))) {
                    page = j + k - 1;
                    for (unsigned long l = 0; l < page_count; ++l) {
                        Memory_Page* x = g_mem_descriptor.pages_struct + page + 1;
                        page_init(x, page_flags);
                        printk(KERN_INFO "Page %ld allocated, physics address: %#018lx\n", page, (g_mem_descriptor.pages_struct + page)->phy_addr);
                    }
                    return (Memory_Page*)(g_mem_descriptor.pages_struct + page);
                }
            }
        }

        printk("Cannot alloc page.\n");
        return NULL;
    }
}

void free_pages(Memory_Page* page,int number)
{

}

bool page_clean(Memory_Page* page)
{

}

Slab_cache* slab_create(unsigned long size, SlabStructor constructor, SlabStructor destructor, unsigned long arg)
{
    Slab_cache* slab_cache = (Slab_cache*)kmalloc(sizeof(Slab_cache), 0);
    if (!slab_cache) {
        printk(KERN_WARNING "Cannot alloc slab_cache\n");
        return NULL;
    }
    memset(slab_cache, 0, sizeof(*slab_cache));
    slab_cache->size = SIZEOF_LONG_ALIGN(size);
    slab_cache->total_using = slab_cache->total_free = 0;
    slab_cache->cache_dma_pool = NULL;
    slab_cache->constructor = constructor;
    slab_cache->destructor = destructor;

    /* allocate slab instance */
    slab_cache->cache_pool = (Slab*)kmalloc(sizeof(Slab), 0);
    list_init(&slab_cache->cache_pool->list);
    slab_cache->cache_pool->page = alloc_pages(zone_normal, 1, 0);
    if (!slab_cache->cache_pool->page) {
        printk(KERN_WARNING "Cannot alloc page\n");
        kfree(slab_cache->cache_pool);
        kfree(slab_cache);
        return NULL;
    }
    page_init(slab_cache->cache_pool->page, PG_Kernel);

    slab_cache->cache_pool->using_count = 0;
    slab_cache->cache_pool->free_count = PAGE_SIZE / slab_cache->size;
    slab_cache->total_free = slab_cache->cache_pool->free_count;
    slab_cache->cache_pool->vaddr = MEM_P2V(slab_cache->cache_pool->page->phy_addr);
    slab_cache->cache_pool->color_count = slab_cache->cache_pool->free_count;
    slab_cache->cache_pool->color_length = (slab_cache->cache_pool->color_count + sizeof(unsigned long) * 8 - 1) >> 6 << 3;
    slab_cache->cache_pool->color_map = (unsigned long*)kmalloc(slab_cache->cache_pool->color_length, 0);
    if (!slab_cache->cache_pool->color_map) {
        printk(KERN_WARNING "Cannot alloc color_map\n");
        kfree(slab_cache->cache_pool);
        kfree(slab_cache);
        return NULL;
    }

    memset(slab_cache->cache_pool->color_map, 0, slab_cache->cache_pool->color_length);
    printk(KERN_INFO "A new slab cache allocated. size: %#018lx, using count: %#018lx, free count: %#018lx, virtual address: %p, color count: %#018lx, color length: %#018lx",
        slab_cache->size, slab_cache->cache_pool->using_count, slab_cache->cache_pool->free_count, slab_cache->cache_pool->vaddr,
        slab_cache->cache_pool->color_count, slab_cache->cache_pool->color_length);
    return slab_cache;
}

bool slab_destroy(Slab_cache* slab_cache)
{
    if (!slab_cache)
        return false;

    Slab* slab = slab_cache->cache_pool;
    if (slab_cache->total_using) {
        /* we still have using slab. Cannot destroy */
        return false;
    }
    while (!list_is_empty(&slab->list)) {
        Slab* tmp = slab;
        slab = container_of(list_next(&slab->list), Slab, list);
        list_erase(&tmp->list);

        /* do some cleaning work*/
        kfree(tmp->color_map);
        page_clean(tmp->page);
        free_pages(tmp->page, 1);
        kfree(tmp);
    }

    kfree(slab->color_map);
    page_clean(slab->page);
    free_pages(slab->page, 1);
    kfree(slab);
    kfree(slab_cache);
    return true;
}

void* slab_malloc(Slab_cache* slab_cache, unsigned long arg)
{
    Slab* slab = slab_cache->cache_pool;
    Slab* tmp = NULL;
    if (!slab_cache->total_free) {
        /* There's no free slab. We need more */
        tmp = (Slab*)kmalloc(sizeof(Slab), 0);
        if (!tmp) {
            printk(KERN_WARNING "Cannot alloc slab\n");
            return NULL;
        }
        memset(tmp, 0, sizeof(Slab));
        list_init(&tmp->list);
        tmp->page = alloc_pages(zone_normal, 1, 0);
        if (!tmp->page) {
            printk(KERN_WARNING "Cannot alloc page\n");
            kfree(tmp);
            return NULL;
        }

        page_init(tmp->page, PG_Kernel);
        tmp->using_count = PAGE_SIZE / slab_cache->size;
        tmp->free_count = tmp->using_count;
        tmp->vaddr = MEM_P2V(tmp->page->phy_addr);
        tmp->color_count = tmp->free_count;
        tmp->color_length = ((tmp->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
        tmp->color_map = (unsigned long*)kmalloc(tmp->color_length, 0);

        if (!tmp->color_map) {
            printk(KERN_WARNING "Cannot alloc color_map\n");
            free_pages(tmp->page, 1);
            kfree(tmp);
            return NULL;
        }

        memset(tmp->color_map, 0, tmp->color_length);
        list_push_back(&slab_cache->cache_pool->list, &tmp->list);
        slab_cache->total_free += tmp->color_count;

        /* initialize color_map */
        for (unsigned long i = 0; i < tmp->color_count; ++i) {
            if ((*(tmp->color_map + (i >> 6)) & (1UL << (i % 64))) == 0) {
                (*(tmp->color_map + (i >> 6)) |= (1UL << (i % 64)));
                ++tmp->using_count;
                --tmp->free_count;
                ++slab_cache->total_using;
                --slab_cache->total_free;
                if (slab_cache->constructor)
                    return slab_cache->constructor((byte*)tmp->vaddr + slab_cache->size * i, arg);

                return (void*)((byte*)tmp->vaddr + slab_cache->size * i);
            }
        }
    } else {
        /* we still have free slab */
        do {
            if (!slab->free_count) {
                slab = container_of(list_next(&slab->list), Slab, list);
                continue;
            }

            for (unsigned long i = 0; i < slab->color_count; ++i) {
                if ((*(slab->color_map + (i >> 6)) & (1UL << (i % 64))) == 0) {
                    (*(slab->color_map + (i >> 6)) |= (1UL << (i % 64)));
                    ++slab->using_count;
                    --slab->free_count;
                    ++slab_cache->total_using;
                    --slab_cache->total_free;
                    if (slab_cache->constructor)
                        return slab_cache->constructor((byte*)slab->vaddr + slab_cache->size * i, arg);

                    return (void*)((byte*)slab->vaddr + slab_cache->size * i);
                }
            }
        } while (slab != slab_cache->cache_pool);
    }

    if(tmp)
    {
        printk(KERN_WARNING "slab_malloc failed.\n");
        list_erase(&tmp->list);
        kfree(tmp->color_map);
        page_clean(tmp->page);
        free_pages(tmp->page,1);
        kfree(tmp);
    }
}

bool slab_free(Slab_cache* slab_cache, void* address,unsigned long arg)
{
    Slab* slab = slab_cache->cache_pool;
    int index = 0;

    do
    {
        if(slab->vaddr <= address && address < slab->vaddr + PAGE_SIZE)
        {
            index = (address - slab->vaddr) / slab_cache->size;
            *(slab->color_map + (index >> 6)) ^= 1UL << index % 64;
            slab->free_count++;
            slab->using_count--;

            slab_cache->total_using--;
            slab_cache->total_free++;
            
            if(slab_cache->destructor != NULL)
            {
                slab_cache->destructor((char *)slab->vaddr + slab_cache->size * index,arg);
            }

            if((slab->using_count == 0) && (slab_cache->total_free >= slab->color_count * 3 / 2))
            {
                list_erase(&slab->list);
                slab_cache->total_free -= slab->color_count;

                kfree(slab->color_map);
                
                page_clean(slab->page);
                free_pages(slab->page,1);
                kfree(slab);
            }

            return true;
        }
        else
        {
            slab = container_of(list_next(&slab->list),Slab,list);
            continue;   
        }       

    } while(slab != slab_cache->cache_pool);

    printk(KERN_INFO "slab_free() ERROR: address not in slab\n");
    return false;
}

bool slab_init()
{
    Memory_Page* page;
    const int count = sizeof(kmalloc_cache_size) / sizeof(kmalloc_cache_size[0]);
    for (unsigned long i = 0; i < count; ++i) {

    }
}

void* kmalloc(unsigned long size, unsigned long gfp_flags)
{

}

bool kfree(void* address)
{

}