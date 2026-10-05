#include "stdint.h"   
#include "print.h"    
#include "thread.h" 
#include "global.h"
#include "syscall-init.h"
#include "syscall.h"

#define syscall_nr 32

typedef void* syscall;
syscall syscall_table[syscall_nr]; // 系统调用表

/* 子功能：获取当前任务 PID */
uint32_t sys_getpid(void) {
    return running_thread()->pid;
}

/* 初始化系统调用表 */
void syscall_init(void) {
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid; // 注册 SYS_GETPID (号为 0)
    put_str("syscall init done\n");
}
