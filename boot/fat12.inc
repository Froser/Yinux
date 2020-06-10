; FAT12基本定义
BS_OEMName              db      'YinuxFlp'      ; OEM厂商
BPB_BytsPerSec          dw      200h            ; 一个扇区的大小 (512字节)
BPB_SecPerClus          db      1               ; 每个簇的扇区数量
BPB_RsvdSecCnt          dw      1               ; 保留扇区数
BPB_NumFATs             db      2               ; FAT表总数
BPB_RootEntCnt          dw      0e0h            ; 根目录文件最大数
BPB_TotSec16            dw      0b40h           ; 扇区总数(2880)
BPB_Media               db      0f0h            ; 媒介描述，软驱(0xF0)
BPB_FATSz16             dw      09h             ; 每个FAT大小
BPB_SecPerTrk           dw      12h             ; 每个磁道扇区数(18)
BPB_NumHeads            dw      2               ; 磁头数
BPB_HiddSec             dd      0               ; 隐藏扇区数
BPB_TotSec32            dd      0               ; 若BPB_TotSec16，此字段表示扇区数
BS_DrvNum               db      0               ; int 13h的驱动器号
BS_Reserved1            db      0               ; 未使用
BS_BootSig              db      29h             ; 扩展引导标记
BS_VolD                 dd      0               ; 卷序列号
BS_VolLab               db      'boot loader'   ; 卷标
BS_FileSysType          db      'FAT12   '      ; 文件系统类型

; 额外定义
SectorsOfRootDir        equ     14              ; 根目录所占扇区
StartSectorOfRootDir    equ     19              ; 根目录起始扇区
SectorFAT1              equ     1               ; FAT1起始扇区