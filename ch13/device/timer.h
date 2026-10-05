#ifndef _DEVICE_TIMER_H
#define _DEVICE_TIMER_H

#include "stdint.h"
/* 定时器初始化函数 */
void timer_init(void);
void mtime_sleep(uint32_t m_seconds);
#endif
