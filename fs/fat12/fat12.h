/* FAT12基本定义 */
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "../../include/yinux/types.h"

typedef struct FS_FAT12_t
{
    DB BS_jmpBoot[3];       // 3字节跳转指令
    DB BS_OEMName[8];       // OEM厂商
    DW BPB_BytesPerSec;     // 一个扇区的大小 (512字节)
    DB BPB_SecPerClus;      // 每个簇的扇区数量
    DW BPB_RsvdSecCnt;      // 保留扇区数
    DB BPB_NumFATs;         // FAT表总数
    DW BPB_RootEntCnt;      // 根目录文件最大数
    DW BPB_TotSec16;        // 扇区总数(2880)
    DB BPB_Media;           // 媒介描述，软驱(0xF0)
    DW BPB_FATSz16;         // 每个FAT大小
    DW BPB_SecPerTrk;       // 每个磁道扇区数(18)
    DW BPB_NumHeads;        // 磁头数
    DD BPB_HiddSec;         // 隐藏扇区数
    DD BPB_TotSec32;        // 若BPB_TotSec16，此字段表示扇区数
    DB BS_DrvNum;           // int 13h的驱动器号
    DB BS_Reserved1;        // 未使用
    DB BS_BootSig;          // 扩展引导标记
    DD BS_VolID;            // 卷序列号
    DB BS_VolLab[8];        // 卷标
    DB BS_FileSysType[11];  // 文件系统类型
} __attribute__((packed)) FS_FAT12;

Yinux_Check_StaticSize(FS_FAT12, 62);

typedef struct FS_FAT12_RootDir_t
{
    DB DIR_Name[11];        // 根目录名
    DB DIR_Attr;            // 文件属性
    DB DIR_Reserved[10];    // 保留
    DW DIR_WrtTime;         // 写入时间
    DW DIR_WrtDate;         // 写入日期
    DW DIR_FstClus;         // 起始簇号
    DD DIR_FileSize;        // 文件大小
} FS_FAT12_RootDir;

Yinux_Check_StaticSize(FS_FAT12_RootDir, 32);

/* 操作FAT12相关函数 */
typedef enum
{
    FS_FAT12_Error_Success = 0,
    FS_FAT12_Error_IO,
    FS_FAT12_Error_Memory,
    FS_FAT12_Error_Size,
} FS_FAT12_CreateError;

typedef struct FS_FAT12_CreateHandle_t
{
    FS_FAT12_CreateError err;
    FILE* file;
    DB* const buffer;
} FS_FAT12_CreateHandle;

FS_FAT12_CreateHandle FS_FAT12_Create(const char* filename);
FS_FAT12_CreateError FS_FAT12_Close(FS_FAT12_CreateHandle* handle);

FS_FAT12_CreateError FS_FAT12_InjectBootFromFile(FS_FAT12_CreateHandle* handle, const char* filename);
FS_FAT12_CreateError FS_FAT12_CreateRootFileFromFileName(FS_FAT12_CreateHandle* handle, const char* filename, const char* fileSource);
FS_FAT12_CreateError FS_FAT12_CreateRootFileFromBinary(FS_FAT12_CreateHandle* handle, const char* filename, const DB* data, size_t fileLength);