#include "print.h"
#include "interrupt.h"
#include "../device/timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "init.h"
#include "keyboard.h"
#include "tss.h"
#include "ide.h"
#include "fs.h"

/* 负责初始化所有模块 */
void init_all(void) {
    put_str("init_all\n");
    intr_init();       // 中断初始化中断
    mem_init();        //内存初始化          8.5.3加
    thread_init();     //线程机制初始化       9.4.4加
    timer_init();      //定时器初始化        7.8加
    console_init();    // 终端初始化         10.2.2加
    keyboard_init();   //键盘初始化          10.3.4加
    tss_init();        //TSS初始化          11.2加
    ide_init();        //硬盘初始化          13.2.2加
    filesys_init();   // 初始化文件系统       14.2.3加
}
