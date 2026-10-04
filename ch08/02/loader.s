%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start

;构建gdt及其内部的描述符
GDT_BASE: dd   0x00000000
          dd   0x00000000

CODE_DESC: dd   0x0000FFFF
           dd   DESC_CODE_HIGH4

DATA_STACK_DESC: dd   0x0000FFFF
                 dd   DESC_DATA_HIGH4

VIDEO_DESC: dd   0x80000007;limit=(0xbffff-0xb8000)/4k=0x7 显存段0x000B8000
            dd   DESC_VIDEO_HIGH4  ;此时dpl为0


GDT_SIZE  equ   $ - GDT_BASE
GDT_LIMIT  equ   GDT_SIZE - 1

times 60 dq 0             ;此处预留60个描述符的空位

SELECTOR_CODE  equ  (0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA  equ  (0x0002<<3) + TI_GDT + RPL0     ;同上
SELECTOR_VIDEO equ  (0x0003<<3) + TI_GDT + RPL0     ;同上

;以下是gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址
gdt_ptr  dw  GDT_LIMIT
         dd  GDT_BASE

loadermsg db '2 loader in real.'

loader_start:
;-----------------------------
;INT 0x10   功能号:0x13   功能描述:打印字符串
mov ax,cs
mov es,ax
mov sp, LOADER_BASE_ADDR   

mov bp, loadermsg          ;ES:BP=字符串地址
mov cx, 17       ;CX=字符串长度
mov ax, 0x1301   ;AH = 13子功能号
                 ;AL=1——字符串中只含显示字符，其显示属性在BL中;显示后，光标位置改变
mov bx, 0x001f   ;页号为0(BH = 0)蓝底粉红字(BL = 1fh)
mov dx, 0x1800   ;坐标
int 0x10         ;10h中断   

;----------------------------------------
;准备进入保护模式:
;1 打开A20
;2 加载gdt
;3 将cr0的pe位置1
;----------------- 打开A20 ----------------
in al, 0x92
or al, 0000_0010B
out 0x92, al
;----------------- 加载GDT -----------------
lgdt [gdt_ptr]
;----------------- cr0第0位置1 -----------------
mov eax, cr0
or eax, 0x00000001
mov cr0, eax

jmp dword SELECTOR_CODE:p_mode_start ;刷新流水线

[bits 32]
p_mode_start:
mov ax, SELECTOR_DATA
mov ds, ax
mov es, ax
mov ss, ax

mov esp, LOADER_STACK_TOP


; --------------step1:加载kernel进内存---------------------------
mov eax, KERNEL_START_SECTOR       ; 内核扇区号
mov ebx, KERNEL_BIN_BASE_ADDR      ; 内存缓冲区
mov ecx, 200                       ; 扇区数
call rd_disk_m_32                  ; 磁盘读取函数


; 初始化内存分页管理
call setup_page                    ; 创建页目录、页表并初始化位图

; 调整全局描述符表(GDT)地址映射
sgdt [gdt_ptr]                     ; 保存当前GDT地址信息

; 更新显存段描述符基址（第3个描述符）
; 将物理地址0x000B8000改为虚拟地址0xC00B8000
mov ebx, [gdt_ptr + 2]                 ; +2跳过gdt_ptr前2字节界限，获取GDT的基地址
or dword [ebx + 0x18 + 4], 0xc0000000  ; 将基址映射到内核空间

; 将GDT基址和栈指针映射到高地址空间
add dword [gdt_ptr + 2], 0xc0000000    ; GDT基址映射
add esp, 0xc0000000                    ; 栈指针重映射


; 启用分页机制
mov eax, PAGE_DIR_TABLE_POS
mov cr3, eax

mov eax, cr0
or eax, 0x80000000
mov cr0, eax                           ; 设置CR0的PG位

; 重新加载更新后的GDT
lgdt [gdt_ptr]

; 重新加载视频段选择子
;mov ax, SELECTOR_VIDEO
;mov gs, ax

; 现在可以安全访问显存
;mov byte [gs:160], 'V'
; 系统挂起
;jmp $

;------------- step2:将kernel.bin中的segment拷贝到编译的地址--------------
jmp SELECTOR_CODE:enter_kernel   
enter_kernel:
    call kernel_init
    mov esp, 0xc009f000           ; 设置内核栈指针
    jmp KERNEL_ENTRY_POINT        ; 跳转到内核入口地址（0xc0001500）

; 内核初始化函数
kernel_init:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    ; 获取程序头表信息
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28]         ; 读取程序头表偏移
    add ebx, KERNEL_BIN_BASE_ADDR                ; 将文件偏移转换为内存地址
    movzx edx,word [KERNEL_BIN_BASE_ADDR + 42]   ; 读取程序头大小
    movzx ecx,word [KERNEL_BIN_BASE_ADDR + 44]   ; 读取程序头数量


.each_segment:
    cmp byte [ebx + 0], PT_NULL    ;PT_NULL为0，表示该程序头未使用，跳过
    je .PTNULL

    ; 复制段到目标地址
    push dword [ebx + 16]    ; 段大小
    mov  eax,  [ebx + 4]     ; 段在文件内的偏移
    add  eax,  KERNEL_BIN_BASE_ADDR
    push eax               ; src
    push dword [ebx + 8]   ; dst 段内有关于该段需被加载到什么地址的信息
    call mem_cpy
    add esp, 12

.PTNULL:
    add ebx, edx
    loop .each_segment
    ret

; 内存复制函数
mem_cpy:
    push ebp
    mov  ebp, esp
    push ecx
    mov  edi, [ebp + 8]   ; dst
    mov  esi, [ebp + 12]  ; src
    mov  ecx, [ebp + 16]  ; size
    cld
    rep movsb
    pop ecx
    pop ebp
    ret


;-------------------------------------------------------------------------------
; 功能: 读取硬盘n个扇区（32位模式）
; 输入: eax = LBA扇区号
;        ebx = 将数据写入的内存地址（32位线性地址）
;        ecx = 读入的扇区数
;-------------------------------------------------------------------------------
rd_disk_m_32:
    push esi
    push edi
    push edx
    push eax
    push ebx
    push ecx

    mov esi, eax       ; 备份LBA扇区号
    mov edi, ecx       ; 备份扇区数

; 第1步：设置要读取的扇区数
    mov dx, 0x1f2
    mov al, cl         ; 扇区数（8位）
    out dx, al

    mov eax, esi       ; 恢复LBA扇区号

; 第2步：将LBA地址存入0x1f3～0x1f6
    ; LBA地址7~0位写入端口0x1f3
    mov dx, 0x1f3
    out dx, al

    ; LBA地址15~8位写入端口0x1f4
    shr eax, 8
    mov dx, 0x1f4
    out dx, al

    ; LBA地址23~16位写入端口0x1f5
    shr eax, 8
    mov dx, 0x1f5
    out dx, al

    ; LBA地址27~24位写入端口0x1f6
    shr eax, 8
    and al, 0x0f      ; 保留低4位（LBA 24-27）
    or al, 0xe0       ; 设置LBA模式，主盘
    mov dx, 0x1f6
    out dx, al

; 第3步：发送读命令（0x20）
    mov dx, 0x1f7
    mov al, 0x20      ; 读扇区命令
    out dx, al

; 第4步：检测硬盘状态
.not_ready:
    in al, dx
    and al, 0x88      ; 检查第3位（DRQ）和第7位（BSY）
    cmp al, 0x08      ; 就绪且不忙？
    jnz .not_ready

; 第5步：从数据端口读取数据
    mov eax, edi       ; 扇区数
    mov ecx, 256       ; 每扇区256字（512字节）
    mul ecx            ; eax = 总字数 = 扇区数 * 256
    mov ecx, eax       ; 设置循环次数

    mov dx, 0x1f0      ; 数据端口
    mov edi, ebx       ; 目标内存地址

; 使用32位内存写入优化循环
.read_loop:
    in ax, dx         ; 从端口读取2字节
    mov [edi], ax     ; 存储到内存
    add edi, 2        ; 移动内存指针
    loop .read_loop   ; 循环直到所有数据读取完成

; 恢复寄存器并返回
    pop ecx
    pop ebx
    pop eax
    pop edx
    pop edi
    pop esi
    ret



;----------------- 页目录及页表初始化 -----------------
setup_page:
    ; 清零页目录区域 一次清空4字节 清空1024次 共4096字节
    xor esi, esi
    mov ecx, 1024           
.clear_dir:
    mov dword [PAGE_DIR_TABLE_POS + esi*4], 0
    inc esi
    loop .clear_dir

    ; 确定第一个页表
    mov eax, PAGE_DIR_TABLE_POS + 0x1000  ; 第一个页表地址
    or eax, PG_US_U | PG_RW_W | PG_P      ; 设置页目录项属性

    ; 映射目录项0和768到同一页表
    mov [PAGE_DIR_TABLE_POS + 0x0],   eax ; 目录项0 →   第一个页表
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax ; 目录项768 → 第一个页表

    ; 设置自映射目录项
    mov eax, PAGE_DIR_TABLE_POS
    or  eax,  PG_US_U | PG_RW_W | PG_P
    mov [PAGE_DIR_TABLE_POS + 4092], eax  ; 目录项1023 → 页目录自身

    ; 初始化第一个页表的256个PTE（一起映射低端1MB内存  物理地址0开始）
    mov edi, PAGE_DIR_TABLE_POS + 0x1000   ; 第一个页表地址
    mov eax, PG_US_U | PG_RW_W |  PG_P     ; 物理地址0 + 属性
    mov ecx, 256                           ; 256个页表项
.init_pte:
    mov [edi], eax
    add edi, 4
    add eax, 4096                         ; 下一物理页
    loop .init_pte

    ; 初始化页目录项769-1022号，内核空间PDE
    mov edi, PAGE_DIR_TABLE_POS + 0x2000    ; 第二个页表开始
    mov ebx, PAGE_DIR_TABLE_POS + 769 * 4   ; 目录项769地址
    mov ecx, 254                            ; 254个目录项
.init_kernel_pde:
    mov eax, edi
    or eax, PG_US_U | PG_RW_W | PG_P
    mov [ebx], eax                        ; 设置页目录项
    add ebx, 4                            ; 下一个目录项
    add edi, 0x1000                       ; 下一个页表
    loop .init_kernel_pde
    ret
