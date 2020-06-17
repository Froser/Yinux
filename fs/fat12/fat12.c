#include "fat12.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const FS_FAT12_RootDir s_emptyRootDir = { 0 };

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

static FS_FAT12_RootDir* availableRootDir(FS_FAT12_CreateHandle* handle)
{
    FS_FAT12* fat12Image = (FS_FAT12*)handle->buffer;
    const int fatTablesSectors = fat12Image->BPB_FATSz16 * fat12Image->BPB_NumFATs;
    const int fatRootDirSectorStart = 1 + fatTablesSectors;
    const int rootEntSectors = (fat12Image->BPB_RootEntCnt * sizeof(FS_FAT12_RootDir) + fat12Image->BPB_BytesPerSec - 1) / fat12Image->BPB_BytesPerSec;

    int offset = fat12Image->BPB_BytesPerSec * fatRootDirSectorStart;
    const FS_FAT12_RootDir* const rootBase = (FS_FAT12_RootDir*)(handle->buffer + offset);
    FS_FAT12_RootDir* rootPtr = (FS_FAT12_RootDir *) rootBase;

    while (memcmp(rootPtr, &s_emptyRootDir, sizeof(s_emptyRootDir)) != 0)
    {
        ++rootPtr;
        if (((DB*)rootPtr - (DB*)rootBase) >= (fat12Image->BPB_BytesPerSec * rootEntSectors)) /* 根目录区已满 */
            return NULL;
    }
    return rootPtr;
}

static void formatFATTables(FS_FAT12_CreateHandle* handle)
{
    FS_FAT12* fat12Image = (FS_FAT12*)handle->buffer;
    const int offsetFAT1SectorStart = 1;
    const int offsetFAT2SectorStart = 1 + fat12Image->BPB_FATSz16;
    DB* fat1Ptr = &handle->buffer[fat12Image->BPB_BytesPerSec * offsetFAT1SectorStart];
    DB* fat2Ptr = &handle->buffer[fat12Image->BPB_BytesPerSec * offsetFAT2SectorStart];
    fat1Ptr[0] = fat2Ptr[0] = fat12Image->BPB_Media;
    fat1Ptr[1] = fat2Ptr[1] = fat1Ptr[2] = fat2Ptr[2] = 0xFF;
}

/* 从FAT的索引获取字节的偏移 */
static int byteOffsetFromFATIndex(int index)
{
    /* 一个FAT表项由1.5个字节表示 */
    return (index + index + index) >> 1;
}

static DW clusterFromFATIndex(DB* imgBuffer, int fatTableOffset, int fatIndex)
{
    DB* ptr = imgBuffer + fatTableOffset + byteOffsetFromFATIndex(fatIndex);
    DB b1 = ptr[0], b2 = ptr[1];
    DW result = 0;
    if (fatIndex & 1)
    {
        /* 如果是奇数，则取第一个字节4bits和第二个字节的低位8bits。第二个字节的8bits作为高位 */
        result = (0xF0 & b1) | (b2 << 4);
    }
    else
    {
        /* 如果是偶数，则取第一个字节8bits和第二个字节的低位4bits。第二个字节的4bits作为高位 */
        result = (b2 & 0x0F) << 8 | b1;
    }
    return result;
}

static void setClusterFromFATIndex(FS_FAT12_CreateHandle* handle, int fatIndex, int cluster)
{
    assert(cluster <= 0xFFF);
    FS_FAT12* fat12Image = (FS_FAT12*)handle->buffer;
    for (int i = 0; i < fat12Image->BPB_NumFATs; ++i)
    {
        int fatTableOffset = (1 + i * fat12Image->BPB_FATSz16) * fat12Image->BPB_BytesPerSec;
        DB* ptr = handle->buffer + fatTableOffset + byteOffsetFromFATIndex(fatIndex);
        DW dwCluster = (DW)cluster;
        if (fatIndex & 1)
        {
            ptr[0] = (dwCluster << 8 >> 8) | (ptr[0] & 0x0F);
            ptr[1] = dwCluster >> 4;
        }
        else
        {
            ptr[0] = dwCluster << 4 >> 4;
            ptr[1] = (ptr[1] & 0xF0) | (dwCluster >> 8);
        }
    }
}

static void availableCluster(FS_FAT12_CreateHandle* handle, int* nextCluster)
{
    /* 遍历根目录，获取下一个簇，并获取其在FAT表中的索引 */
    FS_FAT12* fat12Image = (FS_FAT12*)handle->buffer;
    const int fatTablesSectors = fat12Image->BPB_FATSz16 * fat12Image->BPB_NumFATs;
    const int fatRootDirSectorStart = 1 + fatTablesSectors;
    const int offset = fatRootDirSectorStart * fat12Image->BPB_BytesPerSec;
    FS_FAT12_RootDir* rootPtr = (FS_FAT12_RootDir*) (handle->buffer + offset);

    const int clusterBegin = 2; /* 数据区簇0-1被FAT1和FAT2占用 */
    int cluster = clusterBegin;
    int clusNum = 0;
    while (memcmp(rootPtr, &s_emptyRootDir, sizeof(s_emptyRootDir)) != 0)
    {
        clusNum = rootPtr->DIR_FileSize / 512;
        int clusRemains = rootPtr->DIR_FileSize % 512;
        if (clusRemains > 0)
            ++clusNum;

        cluster += clusNum;

        ++rootPtr;
    }

    if (*nextCluster)
        *nextCluster = cluster;
}

FS_FAT12_CreateHandle FS_FAT12_Create(const char* filename)
{
    FS_FAT12_CreateHandle handle = { FS_FAT12_Error_Success, 0, (DB*)calloc(sizeof(DB), Floppy_Size) };
    handle.file = fopen(filename, "wb");
    if (!handle.file)
    {
        handle.err = FS_FAT12_Error_IO;
        goto done;
    }

    if (!handle.buffer)
    {
        handle.err = FS_FAT12_Error_Memory;
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
            return FS_FAT12_Error_IO;

        fflush(handle->file);
        fclose(handle->file);
        if (handle->buffer)
        {
            free(handle->buffer);
        }
    }
    return FS_FAT12_Error_Success;
}

FS_FAT12_CreateError FS_FAT12_InjectBootFromFile(FS_FAT12_CreateHandle* handle, const char* filename)
{
    FS_FAT12_CreateError err = FS_FAT12_Error_Success;
    FILE* pBoot = fopen(filename, "rb");
    if (!pBoot)
        return FS_FAT12_Error_IO;

    fseek(pBoot, 0, SEEK_END);
    long lSize = ftell(pBoot);
    if (lSize != Floopy_Sector_Size)
    {
        err = FS_FAT12_Error_Size;
        goto done;
    }

    fseek(pBoot, 0, SEEK_SET);

    size_t bytes = fread(handle->buffer, 1, lSize, pBoot);
    if (bytes != lSize)
    {
        err = FS_FAT12_Error_IO;
        goto done;
    }

    /* 初始化FAT表 */
    formatFATTables(handle);

done:
    fclose(pBoot);
    return err;
}

FS_FAT12_CreateError FS_FAT12_CreateRootFileFromFileName(FS_FAT12_CreateHandle* handle, const char* filename, const char* fileSource)
{
    FILE* f = fopen(fileSource, "rb");
    if (!f)
        return FS_FAT12_Error_IO;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    DB* const buf = (DB*)calloc(sizeof(DB), len);
    if (!buf)
        return FS_FAT12_Error_Memory;
    if (len != fread(buf, 1, len, f))
        return FS_FAT12_Error_IO;
    FS_FAT12_CreateError err = FS_FAT12_CreateRootFileFromBinary(handle, filename, buf, len);
    free(buf);
    return err;
}

FS_FAT12_CreateError FS_FAT12_CreateRootFileFromBinary(FS_FAT12_CreateHandle* handle, const char* filename, const DB* data, size_t fileLength)
{
    FS_FAT12* fat12Image = (FS_FAT12*)handle->buffer;
    FS_FAT12_CreateError err = FS_FAT12_Error_Success;
    FS_FAT12_RootDir rootDirFile = { 0 };
    const int fatTablesSectors = fat12Image->BPB_FATSz16 * fat12Image->BPB_NumFATs;
    const int fatRootDirSectorStart = 1 + fatTablesSectors;
    const int rootEntSectors = (fat12Image->BPB_RootEntCnt * sizeof(FS_FAT12_RootDir) + fat12Image->BPB_BytesPerSec - 1) / fat12Image->BPB_BytesPerSec;
    const int dataSectorsStart = fatRootDirSectorStart + rootEntSectors;

    getDOSFilename(filename, rootDirFile.DIR_Name);
    rootDirFile.DIR_FileSize = fileLength;
    rootDirFile.DIR_Attr = 0x20; /* 文件 */

    /* 待写入文件的信息 */
    int clusNum = fileLength / 512;
    int clusRemains = fileLength % 512;
    if (clusRemains > 0)
        ++clusNum;

    if (clusNum > 0)
    {
        /* 获取一个可用的簇号作为起始值 */
        int cluster;
        availableCluster(handle, &cluster);
        int thisCluster = cluster;
        rootDirFile.DIR_FstClus = cluster;

        /* 写根目录 */
        FS_FAT12_RootDir* availableRootDirPtr = availableRootDir(handle);
        if (!availableRootDirPtr)
            return FS_FAT12_Error_Memory;
        memcpy(availableRootDirPtr, &rootDirFile, sizeof(rootDirFile));

        while (clusNum--)
        {
            /* 获取写入的位置。注意，FAT1和FAT2占用了头2个簇 */
            const int offsetData = fat12Image->BPB_BytesPerSec * dataSectorsStart;
            DB* dest = handle->buffer + offsetData + ((cluster - 2) * fat12Image->BPB_BytesPerSec * fat12Image->BPB_SecPerClus);
            DW len = clusRemains ? clusRemains : fat12Image->BPB_BytesPerSec * fat12Image->BPB_SecPerClus;
            memcpy(dest, data, len);

            /* 将cluster设置为下一个簇号，写入FAT表 */
            if (!clusNum)
                cluster = 0xFFF;
            else
                ++cluster;

            setClusterFromFATIndex(handle, thisCluster, cluster);
        }
    }
    return err;
}
