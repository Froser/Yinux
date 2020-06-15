/* 创建一个1.44MB软驱文件，写入引导信息，并且注入一个loader */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/fs/fat12/fat12.h"

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
argv[3]: 写入根目录的loader文件名
argv[4]: 输出文件
*/
int main(int argc, char** argv)
{
    if (argc != 5)
    {
        printf("Arguments count must be 4.");
        return Error_Args;
    }

    printf("Preparing inject %s to %s", argv[2], argv[1]);

    FILE* pImg = fopen(argv[4], "wb");
    if (!pImg)
        return Error_IO;

    char* const imageBuffer = (char*)calloc(sizeof(char), Floppy_Size);
    if (!imageBuffer)
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
    size_t bytes = fread(imageBuffer, 1, lSize, pBoot);
    if (bytes != lSize)
        return Error_IO;
    fclose(pBoot);

    /* 写入引导文件 */
    FILE* pLoader = fopen(argv[2], "rb");
    if (!pLoader)
        return Error_IO;

    fseek(pBoot, 0, SEEK_END);
    lSize = ftell(pLoader);
    fseek(pLoader, 0, SEEK_SET);

    /* 写入根目录 */
    FAT12RootDir rootDirFile = { 0 };
    const char* fnPtr = argv[3];
    int fnlen = strlen(fnPtr);
    if (fnlen > 11)
        fnlen = 11;

    char DOSName[11] = { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, };
    int count = 8;
    while (*fnPtr)
    {
        if (*fnPtr == '.')
        {
            while (--count)
            {
                DOSName[8 - count] = ' ';
            }
            ++fnPtr;
            break;
        }
        DOSName[8 - count] = (*fnPtr++);

        if (!--count)
            break;
    }

    count = 3;
    while (*fnPtr)
    {
        DOSName[11 - count] = (*fnPtr++);
        if (!--count)
            break;
    }

    memcpy(rootDirFile.DIR_Name, DOSName, 11);
    rootDirFile.DIR_FileSize = lSize;
    rootDirFile.DIR_Attr = 0x20; /* 文件 */
    rootDirFile.DIR_FstClus = 0x02; /* 起始簇从2开始 */

    int clusNum = lSize / 512;
    int clusRemains = lSize % 512;
    if (clusRemains > 0)
        ++clusNum;

    FAT12* fat12Image = (FAT12*)imageBuffer;
    const int rootEntSectors = (fat12Image->BPB_RootEntCnt * 32 + fat12Image->BPB_BytesPerSec - 1) / fat12Image->BPB_BytesPerSec;
    const int fatTablesSectors = fat12Image->BPB_FATSz16 * fat12Image->BPB_NumFATs;
    const int dataSectorsStart = 1 + rootEntSectors + fatTablesSectors;

    if (clusNum != 0)
    {
        /* 写根目录 */
        int fatRootDirSectorStart = 1 + fatTablesSectors;
        int offsetRoot = fat12Image->BPB_BytesPerSec * fatRootDirSectorStart;
        memcpy((char*)imageBuffer + offsetRoot, &rootDirFile, sizeof(rootDirFile));

        /* 写数据 */
        char* const loaderBuffer = (char*)calloc(sizeof(char), lSize);
        const int offsetData = fat12Image->BPB_BytesPerSec * dataSectorsStart;
        if (!loaderBuffer)
            return Error_Memory;
        bytes = fread(loaderBuffer, 1, lSize, pLoader);
        if (bytes != lSize)
            return Error_IO;

        memcpy((char*)imageBuffer + offsetData, loaderBuffer, lSize);
        free(loaderBuffer);
        if (bytes != lSize)
            return Error_IO;

        /* 更新FAT表 */
        const int offsetFAT1SectorStart = 1;
        const int offsetFAT2SectorStart = 1 + fat12Image->BPB_FATSz16;
        DB* fat1Ptr = &imageBuffer[fat12Image->BPB_BytesPerSec * offsetFAT1SectorStart];
        DB* fat2Ptr = &imageBuffer[fat12Image->BPB_BytesPerSec * offsetFAT2SectorStart];

        fat1Ptr[0] = fat2Ptr[0] = fat12Image->BPB_Media;
        fat1Ptr[1] = fat2Ptr[1] = fat1Ptr[2] = fat2Ptr[2] = 0xFF;
        fat1Ptr += 2;
        fat2Ptr += 2;

        DD* fat1dd = (DD*)fat1Ptr;
        DD* fat2dd = (DD*)fat2Ptr;

        int clusCnt = clusNum;
        int clusNext = rootDirFile.DIR_FstClus + 1;
        while (clusCnt)
        {
            //TODO
            --clusCnt;
        }
    }

    bytes = fwrite(imageBuffer, 1, Floppy_Size, pImg);
    if (bytes != Floppy_Size)
        return Error_IO;

    fclose(pLoader);

    fflush(pImg);
    free(imageBuffer);
    fclose(pImg);

    return Error_Success;
}