; 低级软驱磁盘服务
%include "fat12.inc"

; 功能：读取磁盘扇区
; ax=起始扇区号
; cl=读取扇区数量
; es:bx=输出地址
FS_ReadSector:

; 原理：
; 通过int 13h, ah=02h读取扇区
; 输入：
; al=读取扇区数
; ch:cl=柱面号:扇区号
; dh=磁头号
; es:bx=数据缓冲区域
; dl=驱动器号

; 返回：
; ah=操作结果
; al=已经读取的扇区
; cf=成功则复位

    push    bp
    mov     bp,     sp
    sub     esp,    2               ; 分配2个字节空间
    mov     byte    [bp - 2],   cl
    push    bx
    mov     bl,     [BPB_SecPerTrk]
    div     bl                      ; 表示al/[BPB_SecPerTrk]，获得al=目标磁头号，ah=起始扇区号 - 1
    inc     ah
    mov     cl,     ah              ; 起始扇区号
    mov     dh,     al
    and     dh,     1               ; 磁头号
    shr     al,     1
    mov     ch,     al              ; 柱面号
    pop     bx
    mov     dl,     [BS_DrvNum]     ; 驱动器号

_FS_ReadSector_Continue:
    mov     ah,     2
    mov     al,     byte    [bp - 2]; 读取扇区数
    int     13h
    jc      _FS_ReadSector_Continue

    add     esp,    2               ; 释放临时变量
    pop     bp
    ret