#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include "stdint.h"

// 修改为接受一个uint8_t参数
typedef void (*intr_handler)(uint8_t vec_nr);
void intr_init(void);

/* 定义中断的两种状态 */
enum intr_status {
    INTR_OFF,    // 中断关闭
    INTR_ON      // 中断打开
};
enum intr_status intr_get_status(void);
enum intr_status intr_set_status(enum intr_status);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);

#endif