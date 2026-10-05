#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "stdint.h"


/* 函数声明 */
void syscall_init(void);
uint32_t sys_getpid(void);
uint32_t sys_write(char* str);

#endif
