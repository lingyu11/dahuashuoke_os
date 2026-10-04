#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"

/* 内存池类型标记 */
enum pool_flags {
    PF_KERNEL = 1,  /* 内核内存池 */
    PF_USER   = 2   /* 用户内存池 */
};

/* 页表项属性标志位 */
#define PG_P_1   1   /* 页表项存在位 */
#define PG_P_0   0   /* 页表项不存在 */
#define PG_RW_R  0   /* 读/执行权限 */
#define PG_RW_W  2   /* 读/写/执行权限 */
#define PG_US_S  0   /* 系统级权限 */
#define PG_US_U  4   /* 用户级权限 */

/* 虚拟地址池结构体 */
struct virtual_addr {
    struct bitmap vaddr_bitmap;  /* 虚拟地址位图结构 */
    uint32_t vaddr_start;        /* 虚拟地址分配起始地址 */
};

extern struct pool kernel_pool, user_pool;

void  mem_init(void);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

#endif 
