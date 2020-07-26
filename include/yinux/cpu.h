#pragma once

#define NR_CPUS 1

void cpuid(unsigned int mainleaf, unsigned int subleaf,
    unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx);

void sys_cpu_init();