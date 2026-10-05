#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H

#include "stdint.h"

enum SYSCALL_NR {
    SYS_GETPID,   // 默认值为 0
    // 后续可添加 SYS_WRITE 等
};

uint32_t getpid(void);

#endif 
