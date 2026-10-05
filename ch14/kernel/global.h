#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#include "stdint.h"

#define default_prio 31 
#define USER_VADDR_START    0x08048000
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define NULL                ((void*)0)
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))
#define true                1
#define false               0

#define PG_SIZE 4096                // 一页4K
#define MEM_BITMAP_BASE 0xc009a000  // 位图基地址
#define K_HEAP_START 0xc0100000     // 内核堆起始地址1M
#define MEM_SIZE 0X2000000          //内存大小32M

/* ==================== 描述符通用属性 ==================== */
#define DESC_G_4K         1     // 粒度4K
#define DESC_G_1B         0     // 粒度1B
#define DESC_D_32         1     // 32位模式
#define DESC_L            0     // 64位代码标记(0=32位)
#define DESC_AVL          0     // CPU保留位
#define DESC_P            1     // 段存在位

/* 特权级 */
#define DESC_DPL_0        0
#define DESC_DPL_1        1
#define DESC_DPL_2        2
#define DESC_DPL_3        3

/* 段类型(S位) */
#define DESC_S_SYS        0            // 系统段(S=0)
#define DESC_S_CODE       1            // 代码段(S=1)
#define DESC_S_DATA       DESC_S_CODE  // 数据段(S=1)

/* 段类型(TYPE) */
#define DESC_TYPE_DATA    2     // 数据段: 不可执行,向上扩展,可写
#define DESC_TYPE_CODE    8     // 代码段: 可执行,非依从,不可读
#define DESC_TYPE_TSS     9     // TSS: 不忙


/* ==================== 选择子定义 ==================== */
/* 请求特权级(RPL) */
#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

/* 表指示符(TI) */
#define TI_GDT 0
#define TI_LDT 1

/* 内核段选择子 */
#define SELECTOR_K_CODE   ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA   ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK  SELECTOR_K_DATA
#define SELECTOR_K_GS     ((3 << 3) + (TI_GDT << 2) + RPL0)  // 显存段
#define SELECTOR_TSS      ((4 << 3) + (TI_GDT << 2) + RPL0)  // TSS

/* 用户段选择子 */
#define SELECTOR_U_CODE   ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA   ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK  SELECTOR_U_DATA

/* ==================== 中断描述符属性 ==================== */
#define IDT_DESC_P        1     // 存在位
#define IDT_DESC_DPL0     0
#define IDT_DESC_DPL3     3
#define IDT_DESC_32_TYPE  0xE  // 32位中断门
#define IDT_DESC_16_TYPE  0x6  // 16位中断门(保留)

/* 中断门属性字节 */
#define IDT_DESC_ATTR_DPL0  ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)
#define IDT_DESC_ATTR_DPL3  ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE)

/* ==================== GDT描述符属性组合 ==================== */
/* 高属性字节(bit 47-40) */
#define GDT_ATTR_HIGH  ((DESC_G_4K << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4))

/* 低属性字节(bit 39-32) - 用户态 */
#define GDT_CODE_ATTR_LOW_DPL3 ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + DESC_TYPE_CODE)
#define GDT_DATA_ATTR_LOW_DPL3 ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_DATA << 4) + DESC_TYPE_DATA)

/* ==================== TSS描述符属性 ==================== */
#define TSS_DESC_D       0     // TSS描述符D位(B位)
#define TSS_ATTR_HIGH    ((DESC_G_1B << 7) + (TSS_DESC_D << 6) + (DESC_L << 5) + (DESC_AVL << 4) + 0x0)
#define TSS_ATTR_LOW     ((DESC_P << 7) + (DESC_DPL_0 << 5) + (DESC_S_SYS << 4) + DESC_TYPE_TSS)


/* ==================== 数据结构定义 ==================== */
struct gdt_desc {
    uint16_t limit_low_word;       // 段限长低16位
    uint16_t base_low_word;        // 基地址低16位
    uint8_t  base_mid_byte;        // 基地址中8位
    uint8_t  attr_low_byte;        // 属性低字节(包括P/DPL/S/TYPE)
    uint8_t  limit_high_attr_high; // 限长高4位 + 属性高4位
    uint8_t  base_high_byte;       // 基地址高8位
};

#define EFLAGS_MBS          (1 << 1)        // 
#define EFLAGS_IF_1         (1 << 9)        // if 为 1，开中断
#define EFLAGS_IF_0         0               // if 为 0，关中断
#define EFLAGS_IOPL_3       (3 << 12)       // IOPL3，用于测试用户程序在非系统调用下进行IO
#define EFLAGS_IOPL_0       (0 << 12)       // IOPL0


#endif

