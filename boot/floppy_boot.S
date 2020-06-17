; 软驱启动传统BISO引导
; BIOS检查第一个扇区是否以0x55aa结尾
; 如果是，则认为是引导扇区
; 接下来将将整个扇区拷贝到物理内存0x7c00处开始执行
; 物理地址的指令寻址方式为CS:IP

; 基址
StackSeg equ 0x0000
StackBase equ 0x7c00

TempSeg equ 0x0000
TempBase equ 0x8000

; 1000:0000 为loader加载地址
LoaderBase equ 0x1000
LoaderOffset equ 0x0000

org 0x7c00

jmp Main                                                    ; 3字节跳转

; 引入
%include "floppy_service.inc"                               ; 引入FAT12描述头，以及软驱磁盘服务
%include "video_service.inc"                                ; 引入低级视频服务

; 常量区
NotFound        db      "Loader not found"
NotFoundLen     equ     $ - NotFound
LoaderName      db      "loader  bin"
LoaderNameLen   equ     $ - LoaderName

Main:                                                       ; cs此时为0x0000，ip此时为0x7c00
    mov         ax,                 cs                      ; 初始化ds, es, ss, sp
    mov         ds,                 ax
    mov         es,                 ax
    mov         bx,                 StackSeg
    mov         ss,                 bx                      ; 堆栈段地址
    mov         sp,                 StackBase               ; 堆栈基地址 ss:sp = 0x17c00
    call        VS_ClearScreen
    call        VS_SetFocus
    jmp         Search_Loader_Begin

; 开始从根目录检索启动程序loader
    sector      dw                  StartSectorOfRootDir    ; 当前根目录搜索扇区
    sectorLoop  dw                  SectorsOfRootDir        ; 当前剩余扇区，用于循环计数

Search_Loader_Begin:
    cmp         word [sectorLoop],  0
    jz          Search_NotFound                             ; 找遍所有根目录扇区，没有找到
    dec         word [sectorLoop]                           ; --sectorLoop
    
    mov         ax,                 TempSeg
    mov         es,                 ax
    mov         bx,                 TempBase
    mov         ax,                 [sector]
    mov         cl,                 1
    call        FS_ReadSector                               ; 从[sector]读取1个扇区，保存到TempSeg:TempBase
    mov         si,                 LoaderName
    mov         di,                 TempBase                ; 临时存放区域
    cld                                                     ; 清除DF，LODSB时递增
    mov         dx,                 10h

Search_Loader_File:
    cmp         dx,                 0
    jz          Search_Next_Sector_RootDir
    dec         dx
    mov         cx,                 11                      ; FAT12根目录部分文件名为11字节。cx作为计数器

Search_Compare_FileName:
    cmp         cx,                 0
    jz          Search_FileName_Found
    dec         cx
    lodsb                                                   ; ds:si中获取字符到al
    cmp         al,                 byte [es:di]
    jz          Search_Compare_GoOn                         ; 单个字符匹配成功
    jmp         Search_Compare_NotMatch                     ; 单个字符匹配失败

Search_Compare_GoOn:
    inc         di
    jmp         Search_Compare_FileName

Search_Compare_NotMatch:
    and         di,                 0ffe0h                  ; 低位置0
    add         di,                 20h                     ; 指向下一个根目录
    mov         si,                 LoaderName
    jmp         Search_Loader_File

Search_Next_Sector_RootDir:
    inc         word [sector]
    jmp         Search_Loader_Begin

Search_NotFound:
    push        ax
    xor         ax,                 ax
    mov         es,                 ax
    pop         ax
    mov         si,                 NotFound
    mov         cx,                 NotFoundLen
    call        VS_Print
    jmp         $                                           ; 死循环

Search_FileName_Found:
    mov         ax,                 TempSeg
    mov         es,                 ax
    mov         ax,                 SectorsOfRootDir        ; 14
    mov         di,                 TempBase                ; [es:di] 为 TempSeg:TempBase，为找到loader的根目录的扇区
    add         di,                 01ah                    ; 获取簇号DIR_FstClus
    mov         cx,                 word [es:di]            ; loader对应的簇
    push        cx
    add         cx,                 ax                      ; +14，
    add         cx,                 17                      ; +17，由于数据区头2个簇被占用了，所以这里只加17 (StartSectorOfRootDir - 2)，此时cx应当为数据所在的簇了
    
    mov         ax,                 LoaderBase
    mov         es,                 ax
    mov         bx,                 LoaderOffset

    mov         ax,                 cx

Loader_Loading:
    mov         cl,                 1
    call        FS_ReadSector                               ; 读取一个扇区的数据，放到es:bx
    pop         ax                                          ; loader对应的簇
    call        FS_GetFATEntry                              ; 下一个簇
    cmp         ax,                 0fffh                   ; 没有下一个簇了，读取已经完成
    jz          Loader_Loaded
    push        ax
    mov         dx,                 SectorsOfRootDir
    add         ax,                 dx
    add         ax,                 17                      ; 读取下一个簇的数据
    add         bx,                 [BPB_BytesPerSec]       ; 偏移BPB_BytesPerSec个字节
    jmp         Loader_Loading

Loader_Loaded:
    jmp         LoaderBase:LoaderOffset                     ; cs->LoaderBase

; 填充扇区
times   510 - ($ - $$) db 0
dw      0xaa55