#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include "stdint.h"

// 修改为接受一个uint8_t参数
typedef void (*intr_handler)(uint8_t vec_nr);
void intr_init(void);

#endif