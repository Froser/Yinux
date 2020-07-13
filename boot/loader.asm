; 内核加载器
org 10000h

; 重新设置栈基址
xor                 ax,             ax
mov                 ss,             ax
mov                 sp,             7c00h
mov                 ax,             1000h
mov                 ds,             ax                      ; 设置段基址

jmp                 Main

%include "floppy_service.inc"
%include "video_service.inc"

TempSeg             equ             0x0000
TempBase            equ             0x8000

NOT_FOUND           db              "Kernel not found"
NotFoundLen         equ             $ - NOT_FOUND
KERNEL_NAME         db              "kernel  bin"
KernelNameLen       equ             $ - KERNEL_NAME

KernelTmpBase       equ             0x0000
KernelTmpOffset     equ             0x7e00
KernelBase          equ             0x00
KernelOffset        equ             0x100000
KernelStart         equ             0x106000

; 开始从根目录检索启动程序kernel
SECTOR              dw              StartSectorOfRootDir    ; 当前根目录搜索扇区
SECTORLOOP          dw              SectorsOfRootDir        ; 当前剩余扇区，用于循环计数
KERNEL_OFS          dd              KernelOffset            ; 用于记录真正的内核偏移

[SECTION gdt]
; 临时GDT
LABEL_GDT:          dd              0, 0                    ; NULL段描述符
LABEL_DESC_CODE32:  dd              0x0000FFFF, 0x00CF9A00  ; 00CF9A000000FFFF
LABEL_DESC_DATA32:  dd              0x0000FFFF, 0x00CF9200  ; 00CF92000000FFFF

GdtLen              equ             $ - LABEL_GDT
SelectorCode32      equ             LABEL_DESC_CODE32 - LABEL_GDT
SelectorData32      equ             LABEL_DESC_DATA32 - LABEL_GDT

; GDT_PTR 表示48位GDT伪描述符 基地址[32]:长度[16]
GDT_PTR             dw              GdtLen - 1
                    dd              LABEL_GDT


[SECTION gdt64]
LABEL_GDT64:        dq              0x0000000000000000
LABEL_DESC_CODE64:  dq              0x0020980000000000
LABEL_DESC_DATA64:  dq              0x0000980000000000

GdtLen64            equ             $ - LABEL_GDT64
SelectorCode64      equ             LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64      equ             LABEL_DESC_DATA64 - LABEL_GDT64

GDT_PTR64           dw              GdtLen64 - 1
                    dd              LABEL_GDT64

[SECTION idt]
; 临时IDT
IDT:
times 0x50          dq              0
IDT_END:
IDT_PTR:
                    dw              IDT_END - IDT - 1
                    dd              IDT

[SECTION .s16]
[BITS 16]
Main:
; 打开A20控制门
; A20控制门表示第20条地址线(从0开始)
; 使用Fast A20 (92h) 来打开A20
    in          al,                 92h                     ; 从端口读取
    or          al,                 2                       ; 修改bit1，设置为打开
    out         92h,                al                      ; 写入端口，打开A20

    cli                                                     ; 禁止中断

    db          0x66
    lgdt        [GDT_PTR]                                   ; 加载GDT表
    mov         eax,                cr0
    or          eax,                1
    mov         cr0,                eax                     ; cr0第一位设置为1，表示进入保护模式

    mov         ax,                 SelectorData32
    mov         fs,                 ax                      ; 修改FS为数据段选择子
    mov         eax,                cr0
    and         al,                 0feh                    ; 退出保护模式
    mov         cr0,                eax

    sti                                                     ; 恢复中断

; 准备寻找kernel文件
Search_Kernel_Begin:
    cmp         word [SECTORLOOP],  0
    jz          Search_NotFound                             ; 找遍所有根目录扇区，没有找到
    dec         word [SECTORLOOP]                           ; --SECTORLOOP
    
    mov         ax,                 TempSeg
    mov         es,                 ax
    mov         bx,                 TempBase
    mov         ax,                 [SECTOR]
    mov         cl,                 1
    call        FS_ReadSector                               ; 从[SECTOR]读取1个扇区，保存到TempSeg:TempBase
    mov         si,                 KERNEL_NAME
    mov         di,                 TempBase                ; 临时存放区域
    cld                                                     ; 清除DF，LODSB时递增
    mov         dx,                 10h

Search_Kernel_File:
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
    mov         si,                 KERNEL_NAME
    jmp         Search_Kernel_File

Search_Next_Sector_RootDir:
    inc         word [SECTOR]
    jmp         Search_Kernel_Begin

Search_NotFound:
    push        ax
    mov         ax,                 ds
    mov         es,                 ax                      ; es和ds同一段
    pop         ax
    mov         si,                 NOT_FOUND
    mov         cx,                 NotFoundLen
    call        VS_Print
    pop         ax
    jmp         $                                           ; 死循环

Search_FileName_Found:
    mov         ax,                 SectorsOfRootDir
    and         di,                 0ffe0h
    add         di,                 01Ah;
    mov         cx,                 word [es:di]
    push        cx
    add         cx,                 ax
    add         cx,                 17
    mov         eax,                KernelTmpBase
    mov         es,                 eax
    mov         ax,                 cx

Kernel_Loading:
    mov         bx,                 KernelTmpOffset
    mov         cl,                 1
    call        FS_ReadSector
    pop         ax                                          ; 对应上面的push cx

    ; 准备将Kernel移到1MB以上的位置
    push        cx
    push        eax
    push        fs
    push        edi
    push        ds
    push        esi

    mov         cx,                 [BPB_BytesPerSec]       ; 一个扇区大小
    mov         ax,                 KernelBase
    mov         fs,                 ax                      ; TODO ONLY Bochs
    mov         edi,                dword [KERNEL_OFS]

    mov         ax,                 KernelTmpBase
    mov         ds,                 ax
    mov         esi,                KernelTmpOffset

Kernel_Mov:
    mov         al,                 byte [ds:esi]           ; 逐个字节从KernelTmpBase:KernelTmpOffset拷贝
    mov         byte [fs:edi],      al                      ; 拷贝到fs:edi
    inc         esi
    inc         edi
    loop        Kernel_Mov
    mov         eax,                1000h
    mov         ds,                 eax
    mov         dword [KERNEL_OFS], edi

    pop         esi
    pop         ds
    pop         edi
    pop         fs
    pop         eax
    pop         cx

    ; 拷贝完成，看看是否有下一个簇
    call        FS_GetFATEntry
    cmp         ax,                 0fffh                   ; 没有下一个簇了，读取已经完成
    jz          Kernel_Loaded
    push        ax
    mov         dx,                 SectorsOfRootDir
    add         ax,                 dx
    add         ax,                 17                      ; 读取下一个簇的数据
    add         bx,                 [BPB_BytesPerSec]       ; 偏移BPB_BytesPerSec个字节
    jmp         Kernel_Loading

Kernel_Loaded:
    ; 至此，内核代码已经移动到了KernelBase:KernelOffset
    ; 之前所有的物理内存均可以废弃

    ; 内存信息
    mov         ebx,                0
    mov         edi,                KernelTmpOffset

Get_Mem_Info:
    mov         eax,                0x0e820
    mov         ecx,                20
    mov         edx,                0x534D4150              ; SMAP
    int         15h
    jc          Mem_Query_Failed
    add         edi,                20
    cmp         ebx,                0
    jne         Get_Mem_Info
    
    ; 准备进入保护模式
    cli                                                     ; 禁止中断

    db          0x66
    lgdt        [GDT_PTR]                                   ; 加载GDT表

    db          0x66
    lidt        [IDT_PTR]                                   ; 加载IDT表
    
    mov         eax,                cr0
    or          eax,                1
    mov         cr0,                eax                     ; cr0第一位设置为1，表示进入保护模式

    jmp         dword SelectorCode32:Temp_Protect           ; 跳转到保护模式的方法中

Mem_Query_Failed:
    jmp         $

[SECTION .s32]
[BITS 32]
Temp_Protect:
    mov         ax,                 10h
    mov         ds,                 ax
    mov         es,                 ax
    mov         fs,                 ax
    mov         ss,                 ax
    mov         esp,                7e00h
    call        SupportLongMode
    test        eax,                eax
    jz          NotSupportLongModeExit
    jmp         Protect_GoOn

SupportLongMode:
    mov         eax,                80000000h               ; 查询支持的最大扩展功能号
    cpuid
    cmp         eax,                80000001h               ; 是否超过0x80000001
    setnb       al
    jb          SupportLongModeFinish                       ; 返回结果
    mov         eax,                80000001h
    cpuid
    bt          edx,                29                      ; EFLAGS第29位表示是否支持长模式
    setc        al

SupportLongModeFinish:
    movzx       eax,                al
    ret

NotSupportLongModeExit:
    jmp         $

Protect_GoOn:
    ; 页表
    mov         dword [0x90000],    0x91007                 ; 0x90000为页表地址
    mov         dword [0x90800],    0x91007

    mov         dword [0x91000],    0x92007

    mov         dword [0x92000],    0x000083
    mov         dword [0x92008],    0x200083
    mov         dword [0x92010],    0x400083
    mov         dword [0x92018],    0x600083
    mov         dword [0x92020],    0x800083
    mov         dword [0x92028],    0xa00083

    db          0x66
    lgdt        [GDT_PTR64]                                 ; 重新加载64位GDT

    ; 开启物理地址扩展 PAE, CR4中的第5位
    mov         eax,                cr4
    bts         eax,                5
    mov         cr4,                eax

    ; 设置页表地址 CR3
    mov         eax,                90000h
    mov         cr3,                eax

    ; 激活IA-32e
    ; 应该用CPUID.01h:EDX[5]来检测
    mov         ecx,                0C0000080h
    rdmsr
    bts         eax,                8
    wrmsr

    ; 开启分页和保护模式(PG, PE)
    mov         eax,                cr0
    bts         eax,                0
    bts         eax,                31
    mov         cr0,                eax

    ; 长跳转到内核文件
    jmp         SelectorCode64:KernelStart
