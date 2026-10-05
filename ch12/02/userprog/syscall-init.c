#include "stdint.h"   
#include "print.h"    
#include "thread.h" 
#include "global.h"
#include "syscall-init.h"
#include "syscall.h"
#include "string.h"
#include "console.h"

#define syscall_nr 32
typedef void* syscall;
syscall syscall_table[syscall_nr]; // 系统调用表

/* 子功能：获取当前任务 PID */
uint32_t sys_getpid(void) {
    return running_thread()->pid;
}

uint32_t sys_write(char* str) {
    console_put_str(str);
    return strlen(str); // 返回写入字符数
}

/* 初始化系统调用表 */
void syscall_init(void) {
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid; // 注册SYS_GETPID(号为 0)
    syscall_table[SYS_WRITE] =  sys_write;
    put_str("syscall init done\n");
}
