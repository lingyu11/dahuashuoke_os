;-----------------  loader和 kernel

LOADER_BASE_ADDR  equ 0x900
LOADER_START_SECTOR  equ 0x2
PAGE_DIR_TABLE_POS equ 0x100000
KERNEL_START_SECTOR  equ 0x9            ;内核磁盘扇区
KERNEL_BIN_BASE_ADDR equ 0x70000        ;内核缓冲区
KERNEL_ENTRY_POINT   equ 0xc0001500     ;内核入口地址
PT_NULL equ 0   

;-----------------  gdt 描述符属性
; 单个位标志
DESC_G_4K    equ     (1 << 23)   ; 粒度位 (G) G=1时，段界限以4KB为单位
DESC_D_32    equ     (1 << 22)   ; 操作数大小位 (D/B)
DESC_L       equ     (0 << 21)   ; 64位代码标志，0表示32位
DESC_AVL     equ     (0 << 20)   ; 可用位

; 段界限字段
DESC_LIMIT_CODE2  equ (0xF << 16)  ; 段界限 bits 16-19
DESC_LIMIT_DATA2  equ DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2 equ (0x0 << 16)  ; 视频段界限 显存段0x000B8000 

; 描述符特权级
DESC_P       equ     (1 << 15)   ; 存在位 P=1表示位于内存
DESC_DPL_0   equ     (0 << 13)   ; DPL = 0
DESC_DPL_1   equ     (1 << 13)   ; DPL = 1  
DESC_DPL_2   equ     (2 << 13)   ; DPL = 2
DESC_DPL_3   equ     (3 << 13)   ; DPL = 3

; 系统标志和类型字段
DESC_S_CODE  equ     (1 << 12)   ; 代码/数据段标志 S=1：代码或数据段；S=0：系统段
DESC_S_DATA  equ     DESC_S_CODE
DESC_S_sys   equ     (0 << 12)   ; 系统段标志

DESC_TYPE_CODE equ   (0x8 << 8)  ; 1000b 代码段类型: EXEC, NON-CONFORMING
DESC_TYPE_DATA equ   (0x2 << 8)  ; 0010b 数据段类型: READ/WRITE, EXPAND-UP

; 高4字节组合
DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
                   DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + \
                   DESC_P + DESC_DPL_0 + DESC_S_CODE + \
                   DESC_TYPE_CODE + 0x00

DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
                   DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + \
                   DESC_P + DESC_DPL_0 + DESC_S_DATA + \
                   DESC_TYPE_DATA + 0x00

;显存段为0x000B8000   所以高4字节为0x000B
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
                    DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + \
                    DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0B
                    


;----------------- 选择子属性----------------
RPL0 equ 0x0
RPL1 equ 0x1  
RPL2 equ 0x2
RPL3 equ 0x3
TI_GDT equ 0x0
TI_LDT equ 0x4
;-----------------页表相关属性----------------
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b
