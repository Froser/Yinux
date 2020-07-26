/* 创建一个1.44MB软驱文件，写入引导信息，并且注入一个loader */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/fs/fat12/fat12.h"

typedef enum
{
    App_Error,
    App_Error_IO = -4,
    App_Error_FileSize = -3,
    App_Error_Memory = -2,
    App_Error_Args = -1,
    App_Error_Success = 0,
} ErrorCode;

enum
{
    Floppy_Size = 512 * 2880,
    Boot_Size = 512,
};

typedef enum
{
    Unknown,
    Input,
    Output,
} State;

typedef struct
{
    const char* src;
    const char* filename;
} InputFileName;

ErrorCode error(FS_FAT12_CreateError err)
{
    switch (err)
    {
    case FS_FAT12_Error_IO:
        printf("IO error occurs.\n");
        return App_Error_IO;
    case FS_FAT12_Error_Memory:
        printf("Memory error occurs.\n");
        return App_Error_Memory;
    case FS_FAT12_Error_Size:
        printf("File wrong size.\n");
        return App_Error_FileSize;
    default:
        return App_Error; /* 不应该到这里来 */
    }
}

/* 主函数 */
/*
argv[1]: 引导信息文件
-i [写入的文件源路径] [FAT12中的文件名]
-o [输出结果]
*/
int main(int argc, char** argv)
{
    if (argc <= 2)
    {
        printf("Arguments count must more than 4.\n");
        return App_Error_Args;
    }

    State state = Unknown;
    const char* output = NULL;
    int current = 0;
    InputFileName inputs[128];
    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "-i") == 0)
        {
            state = Input;
            continue;
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            state = Output;
            continue;
        }

        if (state == Input)
        {
            inputs[current].src = argv[i];
            if (i == argc - 1)
            {
                printf("File name must be specified.\n");
                return App_Error_Args;
            }
            inputs[current].filename = argv[++i];
            ++current;
        }
        else if (state == Output)
        {
            output = argv[i];
        }
        else
        {
            printf("Unrecognized command %s.\n", argv[i]);
            return App_Error_Args;
        }
    }

    printf("Creating floppy image...\n");
    FS_FAT12_CreateHandle handle = FS_FAT12_Create(output);
    if (handle.err == FS_FAT12_Error_IO || handle.err == FS_FAT12_Error_Memory)
        return error(handle.err);

    char* const imageBuffer = handle.buffer;

    /* 将引导信息(512b)放入buffer中 */
    printf("Writing boot data...\n");
    FS_FAT12_CreateError err = FS_FAT12_InjectBootFromFile(&handle, argv[1]);
    if (err == FS_FAT12_Error_IO)
        return error(err);
    
    if (err == FS_FAT12_Error_Size)
        return error(err);

    for (int i = 0; i < current; ++i)
    {
        /* 引导文件写入根目录 */
        printf("Writing %s...\n", inputs[i].filename);
        FS_FAT12_CreateError err = FS_FAT12_CreateRootFileFromFileName(&handle, inputs[i].filename, inputs[i].src);
        if (err == FS_FAT12_Error_IO || err == FS_FAT12_Error_Memory)
            return error(err);
    }

    if ((err = FS_FAT12_Close(&handle)) == FS_FAT12_Error_IO)
    {
        return error(err);
    }

    printf("Done\n");
    return FS_FAT12_Error_Success;
}