[bits 32]
%define ERROR_CODE nop ; 若相关异常CPU已自动压入错误码，为保持栈格式统一，此处不做操作
%define ZERO push 0    ; 若相关异常CPU未压入错误码，为统一栈格式，手工压入0

extern put_str    ; 外部函数声明
extern idt_table  ; 7.6.2 加

section .data
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table

intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry: ; 每个中断处理程序
    %2
    ;保护上下文环境
    push ds
    push es
    push fs
    push gs
    pushad
    
    ; 发送EOI结束中断
    mov al,0x20
    out 0xa0,al
    out 0x20,al
    
    push %1
    call [idt_table+%1 * 4]
    jmp intr_exit

section .data
    dd intr%1entry
%endmacro

section .text
global intr_exit

intr_exit:
   add esp, 4     
   popad
   pop gs
   pop fs         
   pop es
   pop ds
   add esp, 4    
   iretd

VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE
VECTOR 0x0c,ERROR_CODE
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO
VECTOR 0x18,ZERO
VECTOR 0x19,ZERO
VECTOR 0x1a,ZERO
VECTOR 0x1b,ZERO
VECTOR 0x1c,ZERO
VECTOR 0x1d,ZERO
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO
VECTOR 0x20,ZERO ;时钟中断对应入口
VECTOR 0x21,ZERO ;键盘中断对应的入口
VECTOR 0x22,ZERO ;级联用的
VECTOR 0x23,ZERO ;串口2对应的入口
VECTOR 0x24,ZERO ;串口1对应的入口
VECTOR 0x25,ZERO ;并口2对应的入口
VECTOR 0x26,ZERO ;软盘对应的入口
VECTOR 0x27,ZERO ;并口1对应的入口
VECTOR 0x28,ZERO ;实时时钟对应的入口
VECTOR 0x29,ZERO ;重定向
VECTOR 0x2a,ZERO ;保留
VECTOR 0x2b,ZERO ;保留
VECTOR 0x2c,ZERO ;ps/2鼠标
VECTOR 0x2d,ZERO ;fpu浮点单元异常
VECTOR 0x2e,ZERO ;硬盘
VECTOR 0x2f,ZERO ;保留

[bits 32]
; 0x80 号中断处理例程
extern syscall_table
global syscall_handler
syscall_handler:
    ; 1. 保存上下文 (与常规中断一致)
    push 0              ; 占位错误码，保持栈格式统一
    push ds
    push es
    push fs
    push gs
    pushad              ; 压入 EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    push 0x80           ; 占位中断号

    ; 2. 为子功能函数准备参数 (C 调用约定：右向左入栈)
    push edx            ; 第3个参数
    push ecx            ; 第2个参数
    push ebx            ; 第1个参数

    ; 3. 查表调用子功能函数
    ; eax 中存储的是子功能号，每个表项占 4 字节
    call [syscall_table + eax * 4]
    
    add esp, 12         ; 平衡栈，跳过刚才压入的 3 个参数

    ; 4. 将返回值写回内核栈中保存 EAX 的位置
    ; 此时esp指向 push 0x80 的位置。pushad 压入了8个寄存器，EAX是第一个，偏移量为 7*4,
    ;加上push 0x80的4字节，总偏移为 8*4。
    mov [esp + 8 * 4], eax

    ; 5. 恢复上下文并返回
    jmp intr_exit
