; 软驱启动
; BIOS检查第一个扇区是否以0x55aa结尾
; 如果是，则认为是引导扇区
; 接下来将将整个扇区拷贝到物理内存0x7c00处开始执行
; 物理地址的指令寻址方式为CS:IP

; 基址
StackSeg equ 0x0001
StackBase equ 0x7c00
TempSeg equ 0x0002
TempBase equ 0x0000
org 0x7c00

jmp Main
nop                                                     ; 3字节跳转

; 引入
%include "floppy_service.inc"                           ; 引入FAT12描述头，以及软驱磁盘服务
%include "video_service.inc"                              ; 引入低级视频服务

; 常量区
NotFound        db      "Loader not found"
NotFoundLen     equ     $ - NotFound
LoaderName      db      "loader"
LoaderNameLen   equ     $ - LoaderName

Main:                                                   ; cs此时为0x0000，ip此时为0x7c00
mov         ax,                 cs                      ; 初始化ds, es, ss, sp
mov         ds,                 ax
mov         es,                 ax
mov         bx,                 StackSeg
mov         ss,                 bx                      ; 堆栈段地址
mov         sp,                 StackBase               ; 堆栈基地址 ss:sp = 0x17c00
call        VS_ClearScreen
call        VS_SetFocus
jmp         Search_NotFound

; 开始从根目录检索启动程序loader
sector      dw                  StartSectorOfRootDir    ; 当前根目录搜索扇区
sectorLoop  dw                  SectorsOfRootDir        ; 当前剩余扇区，用于循环计数
Search_Loader_Begin:
cmp         word [sectorLoop],  0
call        Search_NotFound                             ; 找遍所有根目录扇区，没有找到
dec         word [sectorLoop]                           ; --sectorLoop

mov         ax,                 TempSeg
mov         es,                 ax
mov         bx,                 TempBase                ; 0x20000 作为临时区域
mov         ax,                 [sector]
mov         cl,                 1
call        FS_ReadSector                               ; 从[sector]读取1个扇区，保存到TempSeg:TempBase (0x20000)
mov         si,                 LoaderName

; TODO 准备开始比较了

jmp         Search_Loader_Begin

Search_NotFound:
push        bp
push        cx
mov         bp,     NotFound
mov         cx,     NotFoundLen
call        VS_Print
pop         cx
pop         bp
jmp         $                                           ; 死循环

; 填充扇区
times   510 - ($ - $$) db 0
dw      0xaa55