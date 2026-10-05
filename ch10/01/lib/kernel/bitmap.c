#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "stdbool.h"

/* 初始化位图,将所有位清零 */
void bitmap_init(struct bitmap* btmp) {
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

/* 检查指定位是否为1*/
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx) {
    uint32_t byte_idx = bit_idx / 8;    // 计算字节索引
    uint32_t bit_odd  = bit_idx % 8;    // 计算位在字节内的偏移
    return ( btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd) );
}


/* 在位图中查找连续cnt个位，返回起始位索引(失败返回-1) */
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
    uint32_t idx_byte = 0;
    // 逐字节查找空闲位
    while (idx_byte < btmp->btmp_bytes_len) {
      if (btmp->bits[idx_byte] != 0xFF) {
           break;
         }
      idx_byte++;
     }

    if (idx_byte  >=  btmp->btmp_bytes_len) {
        return -1;  // 位图已满
    }

    // 在字节内逐位查找空闲位
    int idx_bit = 0;
    while ( (uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte] ) {
        idx_bit++;
    }

    int bit_idx_start = idx_byte * 8 + idx_bit;  // 计算全局位索引
    if (cnt == 1) {
        return bit_idx_start;
    }

    uint32_t bit_left = btmp->btmp_bytes_len * 8 - bit_idx_start;//代表剩余空闲
    uint32_t next_bit = bit_idx_start + 1;    //记录当前位的整体偏移值
    uint32_t count = 1;                       //记录已找到的位的数量
    bit_idx_start = -1;

    // 查找连续cnt个空闲位
    while (bit_left-- > 0) {
        if (!(bitmap_scan_test(btmp, next_bit))) {
            count++;
        } else {
            count = 0;
        }
        if (count == cnt) {
            bit_idx_start = next_bit - cnt + 1;//计算连续空闲位的起始偏移
            break;
        }
        next_bit++;
    }
    return bit_idx_start;
}

/* 设置指定位的值（0或1） */
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) {
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;

    if (value) {
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    } else {
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}
