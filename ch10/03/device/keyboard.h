#ifndef __KEYBOARD_H
#define __KEYBOARD_H
// #include "ioqueue.h"

/* 键盘缓冲寄存器端口号 */
#define KBD_BUF_PORT 0x60

extern struct ioqueue kbd_buf;

/* 键盘初始化函数 */
void keyboard_init(void);

#endif
