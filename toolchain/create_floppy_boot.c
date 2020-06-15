/* 创建一个1.44MB软驱文件，写入引导信息，并且注入一个loader */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/fs/fat12/fat12.h"

enum ErrorCode
{
    App_Error_IO = -4,
    App_Error_FileSize = -3,
    App_Error_Memory = -2,
    App_Error_Args = -1,
    App_Error_Success = 0,
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
        return App_Error_Args;
    }

    printf("Preparing inject %s to %s", argv[2], argv[1]);

    FS_FAT12_CreateHandle handle = FS_FAT12_Create(argv[4]);
    if (handle.err == FS_FAT12_Error_IO)
        return App_Error_IO;

    if (handle.err == FS_FAT12_Error_Memory)
        return App_Error_Memory;

    char* const imageBuffer = handle.buffer;

    /* 将引导信息(512b)放入buffer中 */
    FS_FAT12_CreateError err = FS_FAT12_InjectBootFromFile(&handle, argv[1]);
    if (err == FS_FAT12_Error_IO)
        return App_Error_IO;
    if (err == FS_FAT12_Error_Size)
        return App_Error_FileSize;

    /* 引导文件写入根目录 */
    if (FS_FAT12_Error_Memory == FS_FAT12_CreateRootFileFromFileName(&handle, argv[3], argv[2]))
        return App_Error_Memory;

    if (FS_FAT12_Error_IO == FS_FAT12_Close(&handle))
        return App_Error_IO;

    return FS_FAT12_Error_Success;
}