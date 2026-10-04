; thread/switch.S
[bits 32]
section .text
global switch_to

switch_to:
    push esi
    push edi
    push ebx
    push ebp                      ; 保存当前线程寄存器
    
    mov eax, [esp+20]             ; 获取当前线程PCB地址(参数cur)
    mov [eax], esp                ; 保存栈指针至PCB的self_kstack
    
    mov eax, [esp+24]             ; 获取新线程PCB地址(参数next)
    mov esp, [eax]                ; 加载新线程栈指针
    
    pop ebp
    pop ebx
    pop edi
    pop esi                       ; 恢复新线程寄存器
    ret                           ; 跳转到新线程执行
