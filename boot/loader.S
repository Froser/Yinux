; 内核加载器
jmp                 Main

[SECTION gdt]
; 伪造一个假的GDT
LABEL_GDT:          dd              0, 0
LABEL_DESC_CODE32   dd              0x0000FFFF, 0x00CF9A00
LABEL_DESC_DATA32   dd              0x0000FFFF, 0x00CF9200
GDT_LEN             equ             $ - LABEL_GDT
GDT_PTR             dw              GDT_LEN - 1
                    dd              LABEL_GDT

[SECTION .s16]
[BITS 16]
Main:
    ; 打开A20控制门
    ; A20控制门表示第20条地址线(从0开始)
    ; 使用Fast A20 (92h) 来打开A20
    push            ax
    in              al,             92h                 ; 从端口读取
    or              al,             2                   ; 修改bit1，设置为打开
    out             92h,            al                  ; 写入端口，打开A20

    cli                                                 ; 禁止中断

    db              0x66
    lgdt            [GDT_PTR]                               ; 设置GDT表
    mov             eax,            cr0
    or              eax,            1
    mov             cr0,            eax                 ; cr0第一位设置为1，表示进入保护模式

    mov             ax,             LABEL_DESC_CODE32
    mov             fs,             ax                  ; 修改FS
    mov             eax,            cr0
    and             al,             0feh                ; 退出保护模式
    mov             cr0,            eax

    sti                                                 ; 恢复中断

    jmp             $