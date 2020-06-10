; 低级视频服务

VS_ClearScreen:         ; 清除屏幕
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

VS_SetFocus:            ; 设置光标焦点
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

VS_Print:               ; 显示一个字符串，cx表示字符串长度，es:bp表示字符串内存地址
    push    ax
    push    bx
    push    cx
    push    dx
    mov     ax, 1301h
    mov     bx, 000fh
    mov     dx, 0
    int     10h
    pop     dx
    pop     cx
    pop     bx
    pop     ax
    ret