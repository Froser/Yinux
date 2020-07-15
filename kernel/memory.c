#include "memory.h"
#include <yinux/string.h>

#define PAGE_SHIFT_4K   12      /* PDPTE.PS=0, PDE.PS=1, 2MB page size */
#define PAGE_SIZE_4K    (1ul << PAGE_SHIFT_4K)

#define PAGE_OFFSET     ((unsigned long)0xffff800000000000)

/* Virtual address to physics address */
#define MEM_V2P(addr)   ((unsigned long)(addr) - PAGE_OFFSET)

/* Physics address to virtual address */
#define MEM_P2V(addr)   ((unsigned long*)((unsigned long)(addr) + PAGE_OFFSET))

static Global_Memory_Descriptor g_mem_descriptor;
static unsigned long* g_pml4e;

typedef enum {
    MT_Available = 1,
    MT_Reserved_Invalid,
    MT_ACPI,
    MT_ACPINVS
} Memory_Type;

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

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

void init_memory()
{
    /* see also Kernel.lds */
    g_mem_descriptor.start_code = (unsigned long)&_text;
    g_mem_descriptor.end_code = (unsigned long)&_etext;
    g_mem_descriptor.end_data = (unsigned long)&_edata;
    g_mem_descriptor.end_brk = (unsigned long)&_end;

    printk(KERN_INFO "Section layouts:\n.code: %#018lx-%#018lx\nend of .data:%#018lx\nbrk:%#018lx\n",
        g_mem_descriptor.start_code, g_mem_descriptor.end_code, g_mem_descriptor.end_data, g_mem_descriptor.end_brk);

    int memcount = *((int*)0xffff800000007e00); /* see also: boot/loader.asm Get_Mem_Info */
    Memory_E820* mem = (Memory_E820*)0xffff800000007e04;
    unsigned long availabletotal = 0;
    unsigned long pagetotal = 0;
    g_mem_descriptor.e820_len = memcount;

    printk(KERN_INFO "Memory info blocks (%d):\n", memcount);
    for (int i = 0; i < memcount; ++i) {
        Memory_E820* m = mem + i;
        g_mem_descriptor.e820[i] = *m;

        printk(KERN_INFO "Address: %#010x,%08x\tLength:%#010x,%08x\tType:%s\n", 
            m->e820_32.address2, m->e820_32.address1, m->e820_32.length2, m->e820_32.length1, memtypestr(m->e820_32.type));

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
                z->page_pages_link = 0;
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

    printk(KERN_INFO "Memory descriptor's bits map start at %p, size is %#018lx, length is %#018lx\n",
        g_mem_descriptor.bits_map, g_mem_descriptor.bits_size, g_mem_descriptor.bits_length);
    printk(KERN_INFO "Memory descriptor's pages structs start at %p, size is %#018lx, length is %#018lx\n",
        g_mem_descriptor.pages_struct, g_mem_descriptor.pages_size, g_mem_descriptor.pages_length);
    printk(KERN_INFO "Memory descriptor's zones structs start at %p, size is %#018lx, length is %#018lx\n",
        g_mem_descriptor.zones_struct, g_mem_descriptor.zones_size, g_mem_descriptor.zones_length);
    printk(KERN_INFO "Memory structs end at %#018lx\n", g_mem_descriptor.end_of_struct);

    /* require and set page table */
    g_pml4e = get_pml4e();
    printk(KERN_INFO "Global PML4T: %p\n", g_pml4e);
    printk(KERN_INFO "Global PDPT: %p\n", *MEM_P2V(g_pml4e));
    printk(KERN_INFO "Global PDT: %p\n", *MEM_P2V(*MEM_P2V(g_pml4e)));

    flush_tlb();
}