#include <yinux/cpu.h>
#include <yinux/kernel.h>
#include <yinux/string.h>

void cpuid(unsigned int mainleaf, unsigned int subleaf,
    unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx)
{
    __asm__ __volatile__ (  "cpuid  \n\t"
                            :"=a"(*eax),"=b"(*ebx),"=c"(*ecx),"=d"(*edx)
                            :"0"(mainleaf), "2"(subleaf)
                         );
}

typedef struct {
    char vendor[17];
    char brand[65];
    unsigned int maxextopcode;
    unsigned long phyaddrsize;
    unsigned long linearaddrsize;
    unsigned long familyCode;
    unsigned long extendFamily;
    unsigned long modelNumber;
    unsigned long extendedModel;
    unsigned long processorType;
    unsigned long steppingID;
} CPU_Info;

CPU_Info g_cpu;

void sys_cpu_init()
{
    CPU_Info info = { 0 };

    unsigned int cpuFac[4] = { 0 };
    /* get vendor */
    cpuid(0, 0, &cpuFac[0], &cpuFac[1], &cpuFac[2], &cpuFac[3]);
    *(unsigned int*)&g_cpu.vendor[0] = cpuFac[1];
    *(unsigned int*)&g_cpu.vendor[4] = cpuFac[3];
    *(unsigned int*)&g_cpu.vendor[8] = cpuFac[2];

    unsigned long offset = 0;
    for (unsigned long i = 0x80000002; i < 0x80000005; ++i) {
        cpuid(i, 0, &cpuFac[0], &cpuFac[1], &cpuFac[2], &cpuFac[3]);
        *(unsigned int*)&g_cpu.brand[offset + 0] = cpuFac[0];
        *(unsigned int*)&g_cpu.brand[offset + 4] = cpuFac[1];
        *(unsigned int*)&g_cpu.brand[offset + 8] = cpuFac[2];
        *(unsigned int*)&g_cpu.brand[offset + 12] = cpuFac[3];
        offset += 16;
    }

    /* get processor info */
    cpuid(1, 0, &cpuFac[0], &cpuFac[1], &cpuFac[2], &cpuFac[3]);
    g_cpu.familyCode = (cpuFac[0] >> 8 & 0xf);
    g_cpu.extendFamily = (cpuFac[0] >> 20 & 0xff);
    g_cpu.modelNumber = (cpuFac[0] >> 4 & 0xf);
    g_cpu.extendedModel = (cpuFac[0] >> 16 & 0xf);
    g_cpu.processorType = (cpuFac[0] >> 12 & 0x3);
    g_cpu.steppingID = cpuFac[0] & 0xf;

    /* linear/physical address size */
    cpuid(0x80000008, 0, &cpuFac[0], &cpuFac[1], &cpuFac[2], &cpuFac[3]);
    g_cpu.phyaddrsize = cpuFac[0] & 0xff;
    g_cpu.linearaddrsize = (cpuFac[0] >> 8) & 0xff;

    /* max extended operation code */
    cpuid(0x80000000, 0, &cpuFac[0], &cpuFac[1], &cpuFac[2], &cpuFac[3]);
    g_cpu.maxextopcode = cpuFac[0];

    printk(KERN_INFO "CPU vendor: %s\nCPU brand: %s\nMax operation code: %#010x\nPhysical address size:  %#ld\nLinear address size: %#ld\n"
        "Family code: %ld\nExtend family: %ld\nModel number: %ld\nExtended model: %ld\nProcessor type: %ld\nStepping ID: %ld\n",
        g_cpu.vendor, g_cpu.brand, g_cpu.maxextopcode, g_cpu.phyaddrsize, g_cpu.linearaddrsize, 
        g_cpu.familyCode, g_cpu.extendFamily, g_cpu.modelNumber, g_cpu.extendedModel, g_cpu.processorType, g_cpu.steppingID);
}