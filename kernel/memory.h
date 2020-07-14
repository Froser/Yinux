#pragma once
#include <yinux/kernel.h>

void init_memory();

typedef struct
{
    unsigned int address1;
    unsigned int address2;
    unsigned int length1;
    unsigned int length2;
} Memory_E820_Formate;