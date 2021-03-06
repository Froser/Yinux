; 低级视频服务

; 功能：清除屏幕
VS_ClearScreen:
    push    ax
    push    bx
    push    cx
    push    dx
    mov     ax, 0600h
    mov     bx, 0700h
    mov     cx, 0
    mov     dx, 0184fh
    int     10h
    pop     dx
    pop     cx
    pop     bx
    pop     ax
    ret

; 功能：设置光标焦点
VS_SetFocus:
    push    ax
    push    bx
    push    dx
    mov     ax, 0200h
    mov     bx, 0
    mov     dx, 0
    int     10h
    pop     dx
    pop     bx
    pop     ax
    ret

; 功能：显示一个字符串
; cx=字符串长度
; es:si=字符串内存地址
VS_Print:
    push    bp
    mov     bp,     sp
    push    si
    push    ax
    push    bx
    push    dx
    mov     bp,     si          ; int 10h中es:bp表示目的位置
    mov     ax,     1301h
    mov     bx,     000fh
    mov     dx,     0
    int     10h
    pop     si
    pop     dx
    pop     bx
    pop     ax
    pop     bp
    ret