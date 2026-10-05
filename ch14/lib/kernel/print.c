TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

[bits 32]
section .text
global put_char

put_char:
    pushad                     ; 备份寄存器
    mov ax, SELECTOR_VIDEO     ; 设置视频段选择子
    mov gs, ax

    ; 获取光标位置（高8位）
    mov dx, 0x3D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x3D5
    in al, dx
    mov ah, al

    ; 获取光标位置（低8位）
    mov dx, 0x3D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x3D5
    in al, dx

    ; 光标值存入bx
    mov bx, ax
    mov ecx, [esp+36]          ; 获取传入的字符

    ; 判断字符类型
    cmp cl, 0x0D               ; 回车符
    jz .carriage_return
    cmp cl, 0x0A               ; 换行符
    jz .line_feed
    cmp cl, 0x08               ; 退格符
    jz .backspace
    
    jmp .printable

;退格
.backspace:
    dec bx                     ; 光标前移
    shl bx, 1                  ; 转换为显存偏移
    mov byte [gs:bx], 0x20     ; 写入空格
    inc bx
    mov byte [gs:bx], 0x07     ; 属性字节
    shr bx, 1
    jmp .set_cursor

;打印可见字符
.printable:
    shl bx, 1
    mov [gs:bx], cl            ; 写入字符
    inc bx
    mov byte [gs:bx], 0x07     ; 属性字节
    shr bx, 1
    inc bx                     ; 光标后移
    cmp bx, 2000               ; 是否需滚屏
    jl .set_cursor

;回车换行处理
.line_feed:
.carriage_return:
    ; 处理换行/回车：光标移到下一行首
    xor dx, dx
    mov ax, bx
    mov si, 80
    div si                     ; bx/80，商在ax，余数在dx
    sub bx, dx                 ; 回车：回到行首
    add bx, 80                 ; 换行：移到下一行
    cmp bx, 2000
    jl .set_cursor

;滚屏处理
.roll_screen:
    ; 滚屏：第1-24行内容上移，清空最后一行
    cld
    mov ecx, 960               ; 3840字节/4字节
    mov edi, 0xC00B8000        ; 第1行起始
    mov esi, 0xC00B80A0        ; 第2行起始
    rep movsd                  ; 复制内存
;滚屏时最后一行空白处理
    mov ebx, 3840              ; 最后一行起始偏移
    mov ecx, 80
.clear_line:
    mov word [gs:ebx], 0x0720  ; 黑底白字空格
    add ebx, 2
    loop .clear_line
    mov bx, 1920               ; 光标置最后一行首

;更新光标位置
.set_cursor:
    ; 更新光标位置寄存器
    mov dx, 0x3D4
    mov al, 0x0E
    out dx, al
    
    mov dx, 0x3D5
    mov al, bh
    out dx, al                 ; 写入高8位

    mov dx, 0x3D4
    mov al, 0x0F
    out dx, al
    
    mov dx, 0x3D5
    mov al, bl
    out dx, al                 ; 写入低8位

    popad                      ; 恢复寄存器
    ret
    
    
  
[bits 32]
section .text
global put_str
put_str:
    push ebx
    push ecx
    xor ecx, ecx
    mov ebx, [esp+12]          ; 获取字符串地址

.next_char:
    mov cl, [ebx]
    cmp cl, 0                   ; 遇到\0结束
    jz .done
    push ecx                    ; 参数入栈
    call put_char
    add esp, 4                  ; 清理栈
    inc ebx                     ; 下一个字符
    jmp .next_char

.done:
    pop ecx
    pop ebx
    ret


section .data
    put_int_buffer dq 0         ; 8字节缓冲区

section .text
global put_int

put_int:
    ; 保存所有寄存器
    pushad
    mov ebp, esp
    
    ; 获取参数（跳过8个pushad的寄存器和返回地址）
    mov eax, [ebp + 4 * 9]
    
    ; 初始化
    mov edx, eax                  ; 保存原始值
    mov edi, 7                    ; 缓冲区偏移，从高位开始
    mov ecx, 8                    ; 8个十六进制数字
    mov ebx, put_int_buffer      ; 缓冲区地址
    
    ; 将缓冲区清零
    mov dword [ebx], 0
    mov dword [ebx+4], 0
    
; 转换循环：将32位整数转为十六进制字符
.convert_loop:
    and edx, 0x0000000F          ; 取最低4位
    cmp edx, 9
    jg .hex_letter               ; 如果大于9，转为字母
    
    ; 数字0-9
    add edx, '0'                 ; 转为'0'-'9'
    jmp .store
    
.hex_letter:
    ; 字母A-F
    sub edx, 10
    add edx, 'A'                 ; 转为'A'-'F'
    
.store:
    ; 将字符存入缓冲区
    mov [ebx + edi], dl
    dec edi                     ; 移到下一个位置
    shr eax, 4                 ; 右移4位，处理下一个十六进制数字
    mov edx, eax
    loop .convert_loop
    
; edi=-1 从头开始扫描 跳过高位零
    inc edi
    cmp edi, 8
    je .full_zero               ; 如果全零，跳转到全零处理
    
.skip_zeros:
    ; 查找第一个非零字符
    mov cl, [ebx + edi]
    cmp cl, '0'
    jne .print                  ; 找到非零字符，开始打印
    inc edi
    cmp edi, 8
    jl .skip_zeros              ; 继续查找
    
    ; 如果到这里，说明全是零
    jmp .full_zero
    
.full_zero:
    ; 处理全零情况：输出单个'0'
    push dword '0'              ; 压入字符'0'
    call put_char               ; 调用put_char输出
    add esp, 4                  ; 清理栈
    jmp .end_function           ; 跳转到函数结束
    
.print:
    ; 输出十六进制字符串
.print_loop:
    movzx ecx, byte [ebx + edi] ; 获取字符（零扩展）
    push ecx                    ; 压入参数
    call put_char               ; 调用put_char输出字符
    add esp, 4                  ; 清理栈
    
    inc edi                     ; 移到下一个字符
    cmp edi, 8
    jl .print_loop              ; 继续输出直到8个字符都处理完
    
.end_function:
    ; 恢复所有寄存器
    popad
    ret

global set_cursor
set_cursor:
    pushad                     ; 保存所有寄存器（与 put_char 一致）
    mov bx, [esp + 36]         ; cdecl: 参数 = pushad(32B) + ret_addr(4B) = 36
    
    ; --- 更新光标位置（与 .set_cursor 局部标签代码完全一致）---
    mov dx, 0x3D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x3D5
    mov al, bh
    out dx, al

    mov dx, 0x3D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x3D5
    mov al, bl
    out dx, al
    ; ---------------------------------------------------------

    popad
    ret                        ; cdecl: 调用者清理栈
