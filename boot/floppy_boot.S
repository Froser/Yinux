; 软驱启动
; BIOS检查第一个扇区是否以0x55aa结尾
; 如果是，则认为是引导扇区
; 接下来将将整个扇区拷贝到物理内存0x7c00处开始执行
; 物理地址的寻址方式为CS:IP

; 基址
StackSeg equ 0x0001
StackBase equ 0x7c00
org 0x7c00

; 以下为FAT12文件系统格式
jmp Main
nop                                     ; 3字节跳转
BS_OEMName      db      'YinuxFlp'      ; OEM厂商
BPB_BytsPerSec  dw      200h            ; 一个扇区的大小 (512字节)
BPB_SecPerClus  db      1               ; 每个簇的扇区数量
BPB_RsvdSecCnt  dw      1               ; 保留扇区数
BPB_NumFATs     db      2               ; FAT表总数
BPB_RootEntCnt  dw      0e0h            ; 根目录文件最大数
BPB_TotSec16    dw      0b40h           ; 扇区总数(2880)
BPB_Media       db      0f0h            ; 媒介描述，软驱(0xF0)
BPB_FATSz16     dw      09h             ; 每个FAT大小
BPB_SecPerTrk   dw      12h             ; 每个磁道扇区数(18)
BPB_NumHeads    dw      2               ; 磁头数
BPB_HiddSec     dd      0               ; 隐藏扇区数
BPB_TotSec32    dd      0               ; 若BPB_TotSec16，此字段表示扇区数
BS_DrvNum       db      0               ; int 13h的驱动器号
BS_Reserved1    db      0               ; 未使用
BS_BootSig      db      29h             ; 扩展引导标记
BS_VolD         dd      0               ; 卷序列号
BS_VolLab       db      'boot loader'   ; 卷标
BS_FileSysType  db      'FAT12   '      ; 文件系统类型

; 引入
%include "video_service.S"
%include "floppy_service.S"

; 数据
STR_Welcome     db  "Welcome To Yinux"
LEN_Welcome     equ $ - STR_Welcome


Main:                                   ; cs此时为0x0000，ip此时为0x7c00
mov     ax,     cs                      ; 初始化ds, es, ss, sp
mov     ds,     ax
mov     es,     ax
mov     bx,     StackSeg
mov     ss,     bx                      ; 堆栈段地址
mov     sp,     StackBase               ; 堆栈基地址 ss:sp = 0x17c00
call    VS_ClearScreen
call    VS_SetFocus

push    bp
push    cx
mov     bp,     STR_Welcome
mov     cx,     LEN_Welcome
call    VS_Print                        ; 打印欢迎提示
pop     cx
pop     bp

; 填充扇区
times   510 - ($ - $$) db 0
dw      0xaa55