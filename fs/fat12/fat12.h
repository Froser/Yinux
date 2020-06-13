/* FAT12基本定义 */
#include "../../include/yinux/types.h"

typedef struct FAT12_t
{
    DB BS_OEMName      ; //OEM厂商
    DW BPB_BytesPerSec ; //一个扇区的大小 (512字节)
    DB BPB_SecPerClus  ; //每个簇的扇区数量
    DW BPB_RsvdSecCnt  ; //保留扇区数
    DB BPB_NumFATs     ; //FAT表总数
    DW BPB_RootEntCnt  ; //根目录文件最大数
    DW BPB_TotSec16    ; //扇区总数(2880)
    DB BPB_Media       ; //媒介描述，软驱(0xF0)
    DW BPB_FATSz16     ; //每个FAT大小
    DW BPB_SecPerTrk   ; //每个磁道扇区数(18)
    DW BPB_NumHeads    ; //磁头数
    DD BPB_HiddSec     ; //隐藏扇区数
    DD BPB_TotSec32    ; //若BPB_TotSec16，此字段表示扇区数
    DB BS_DrvNum       ; //int 13h的驱动器号
    DB BS_Reserved1    ; //未使用
    DB BS_BootSig      ; //扩展引导标记
    DD BS_VolD         ; //卷序列号
    DB BS_VolLab       ; //卷标
    DB BS_FileSysType  ; //文件系统类型
} FAT12;