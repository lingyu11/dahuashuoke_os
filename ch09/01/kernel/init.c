#include "print.h"
#include "interrupt.h"
#include "init.h"
#include "../device/timer.h"
#include "memory.h"

/* 负责初始化所有模块 */
void init_all() {
    put_str("init_all\n");
    intr_init();  // 中断初始化中断
    timer_init(); // 定时器初始化     7.8加
    mem_init();   // 内存初始化       8.5.3加
}