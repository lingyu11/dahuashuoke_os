#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H

#include "global.h"
#include "stdbool.h"

#define BITMAP_MASK 1  // 位操作掩码

struct bitmap {
    uint32_t btmp_bytes_len;  // 位图字节长度
    uint8_t* bits;            // 位图指针(单字节类型)
};

//通过memset清零位图
void bitmap_init(struct bitmap* btmp);

//检查特定位是否被占用
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);

//核心函数，用于查找连续空闲位
int  bitmap_scan(struct bitmap* btmp, uint32_t cnt);

//设置特定位设为1(占用)或0(空闲)
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);

#endif
