/* 创建一个1.44MB软驱文件，写入引导信息，并且注入一个loader */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "../include/fs/fat12/fat12.h"

typedef struct _FAT12_HEADER FAT12_HEADER;
typedef struct _FAT12_HEADER* PFAT12_HEADER;

enum ErrorCode
{
    Error_IO = -4,
    Error_FileSize = -3,
    Error_Memory = -2,
    Error_Args = -1,
    Error_Success = 0,
};

enum
{
    Floppy_Size = 512 * 2880,
    Boot_Size = 512,
};

/* 主函数 */
/*
argv[1]: 引导信息文件
argv[2]: loader文件
argv[3]: 输出文件
*/
int main(int argc, char** argv)
{
    if (argc != 4)
    {
        printf("Arguments count must be 3.");
        return Error_Args;
    }

    printf("Preparing inject %s to %s", argv[2], argv[1]);

    FILE* pImg = fopen(argv[3], "wb");
    if (!pImg)
        return Error_IO;

    char* buffer = (char*)calloc(sizeof(char), Floppy_Size);
    if (!buffer)
        return Error_Memory;

    /* 将引导信息(512b)放入buffer中 */
    FILE* pBoot = fopen(argv[1], "rb");
    if (!pBoot)
        return Error_IO;

    fseek(pBoot, 0, SEEK_END);
    long lSize = ftell(pBoot);
    if (lSize != 512)
        return Error_FileSize;
    fseek(pBoot, 0, SEEK_SET);
    size_t bytes = fread(buffer, 1, lSize, pBoot);
    if (bytes != lSize)
        return Error_IO;
    fclose(pBoot);

    bytes = fwrite(buffer, 1, Floppy_Size, pImg);
    if (bytes != Floppy_Size)
        return Error_IO;

    fflush(pImg);
    free(buffer);
    fclose(pImg);

    return Error_Success;
}