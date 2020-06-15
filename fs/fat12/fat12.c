#include "fat12.h"
#include <stdlib.h>
#include <string.h>

enum
{
    Floopy_Sector_Size = 512,
    Floppy_Size = Floopy_Sector_Size * 2880,
};

static const char* getDOSFilename(const char* filename, char resultBuffer[11])
{
    const char* fnPtr = filename;
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

    memcpy(resultBuffer, DOSName, sizeof(DOSName));
    return resultBuffer;
}

static int availableRootDirOffset(FS_FAT12_CreateHandle* handle)
{
    static const FS_FAT12_RootDir emptyRootDir = { 0 };
    FS_FAT12* fat12Image = (FS_FAT12*)handle->buffer;
    const int fatTablesSectors = fat12Image->BPB_FATSz16 * fat12Image->BPB_NumFATs;
    const int fatRootDirSectorStart = 1 + fatTablesSectors;
    const int rootEntSectors = (fat12Image->BPB_RootEntCnt * sizeof(FS_FAT12_RootDir) + fat12Image->BPB_BytesPerSec - 1) / fat12Image->BPB_BytesPerSec;
    int offset = fat12Image->BPB_BytesPerSec * fatRootDirSectorStart;
    while (memcmp((char*)handle->buffer + offset, &emptyRootDir, sizeof(emptyRootDir)) != 0)
    {
        offset += sizeof(emptyRootDir);
        if (offset >= (fat12Image->BPB_BytesPerSec * rootEntSectors)) /* 根目录区已满 */
            return -1;
    }
    return offset;
}

FS_FAT12_CreateHandle FS_FAT12_Create(const char* filename)
{
    FS_FAT12_CreateHandle handle = { Error_Success, 0, (DB*)calloc(sizeof(DB), Floppy_Size) };
    handle.file = fopen(filename, "wb");
    if (!handle.file)
    {
        handle.err = Error_IO;
        goto done;
    }

    if (!handle.buffer)
    {
        handle.err = Error_Memory;
        goto done;
    }

done:
    return handle;
}

FS_FAT12_CreateError FS_FAT12_Close(FS_FAT12_CreateHandle* handle)
{
    if (handle && handle->file)
    {
        size_t bytes = fwrite(handle->buffer, 1, Floppy_Size, handle->file);
        if (bytes != Floppy_Size)
            return Error_IO;

        fflush(handle->file);
        fclose(handle->file);
        if (handle->buffer)
        {
            free(handle->buffer);
        }
    }
    return Error_Success;
}

FS_FAT12_CreateError FS_FAT12_InjectBootFromFile(FS_FAT12_CreateHandle* handle, const char* filename)
{
    FS_FAT12_CreateError err = Error_Success;
    FILE* pBoot = fopen(filename, "rb");
    if (!pBoot)
        return Error_IO;

    fseek(pBoot, 0, SEEK_END);
    long lSize = ftell(pBoot);
    if (lSize != Floopy_Sector_Size)
    {
        err = Error_Size;
        goto done;
    }

    fseek(pBoot, 0, SEEK_SET);

    size_t bytes = fread(handle->buffer, 1, lSize, pBoot);
    if (bytes != lSize)
    {
        err = Error_IO;
        goto done;
    }

done:
    fclose(pBoot);
    return err;
}

FS_FAT12_CreateError FS_FAT12_CreateRootFileFromFileName(FS_FAT12_CreateHandle* handle, const char* filename, const char* fileSource)
{
    FILE* f = fopen(fileSource, "rb");
    if (!f)
        return Error_IO;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    DB* const buf = (DB*)calloc(sizeof(DB), len);
    FS_FAT12_CreateError err = FS_FAT12_CreateRootFileFromBinary(handle, filename, buf, len);
    free(buf);
    return err;
}

FS_FAT12_CreateError FS_FAT12_CreateRootFileFromBinary(FS_FAT12_CreateHandle* handle, const char* filename, const DB* data, size_t fileLength)
{
    FS_FAT12_CreateError err = Error_Success;
    FS_FAT12_RootDir rootDirFile = { 0 };
    getDOSFilename(filename, rootDirFile.DIR_Name);
    rootDirFile.DIR_FileSize = fileLength;
    rootDirFile.DIR_Attr = 0x20; /* 文件 */
    //rootDirFile.DIR_FstClus = 0x02; /* 起始簇从2开始 */

    /* 待写入文件的信息 */
    int clusNum = fileLength / 512;
    int clusRemains = fileLength % 512;
    if (clusRemains > 0)
        ++clusNum;

    if (clusNum != 0)
    {
        /* 写根目录 */
        int rootDirOffset = availableRootDirOffset(handle);
        if (rootDirOffset == -1)
            return Error_Memory;
        memcpy((char*)handle->buffer + rootDirOffset, &rootDirFile, sizeof(rootDirFile));

        /* 写数据 */
        //const int offsetData = fat12Image->BPB_BytesPerSec * dataSectorsStart;
        //memcpy((char*)handle->buffer + offsetData, data, fileLength);

        /* 更新FAT表 
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
        */
    }
    return err;
}
