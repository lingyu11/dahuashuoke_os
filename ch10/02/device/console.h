#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "stdint.h"  

/* 初始化终端锁 */
void console_init(void);
/* 获取终端锁（加锁） */
void console_acquire(void);
/* 释放终端锁（解锁） */
void console_release(void);
/* 安全输出字符串 */
void console_put_str(char* str);
/* 安全输出单个ASCII字符 */
void console_put_char(uint8_t char_asci);
/* 安全输出32位无符号整数（十六进制） */
void console_put_int(uint32_t num);

#endif
