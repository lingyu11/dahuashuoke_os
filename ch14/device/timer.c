#include "timer.h"
#include "io.h"
#include "print.h"
#include "thread.h"  
#include "interrupt.h" 
#include "debug.h" 
#include "console.h" 


#define PIT_CONTROL_PORT 0x43       //控制字寄存器端口


#define CONTRER0_PORT 0x40          //计数器0端口
#define COUNTER0_NO  0              //00  表选控制计数器0
#define READ_WRITE_LATCH 3          //11  先读写低字节 再读写高字节
#define COUNTER_MODE 2              //010 工作方式2


#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY

#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)     //12.2.3加


uint32_t ticks;           // ticks 是内核自中断开启以来总共的嘀嗒数

/* 设置控制寄存器、初始化计数器0的初始值 */
static void frequency_set (uint8_t counter_port, uint8_t counter_no, uint8_t rw1,uint8_t counter_mode, uint16_t counter_value) {
  
  outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rw1 << 4 | counter_mode << 1));
  outb (counter_port, (uint8_t)counter_value);     //先写入counter_value的低8位
  outb (counter_port, (uint8_t)counter_value >> 8);//再写入counter_value的高8位
}

/* 时钟的中断处理函数 */
static void intr_timer_handler(uint8_t vector_no) {
    struct task_struct* cur_thread = running_thread();
    ASSERT(cur_thread->stack_magic == 0x19870916); // 检查栈是否溢出
    cur_thread->elapsed_ticks++;  // 记录此线程占用的 cpu 时间
    ticks++;                      // 从内核第一次处理时间中断后至今的滴哒数，内核态和用户态总合

    if (cur_thread->ticks == 0) { // 若进程时间片用完，就开始调度新的进程上 cpu
        schedule();
    } else {                      // 将当前进程的时间片-1
        cur_thread->ticks--;
    }
}

/* 初始化 PIT8253 */
void timer_init() {
    put_str("timer_init start\n");
    /* 设置 8253 的定时周期，也就是发中断的周期 */
    frequency_set(CONTRER0_PORT, \
                  COUNTER0_NO, \
                  READ_WRITE_LATCH, \
                  COUNTER_MODE, \
                  COUNTER0_VALUE);
    register_handler(0x20, intr_timer_handler);
    put_str("timer_init done\n");
}


/* 以tick为单位的sleep, 任何时间形式的sleep会转换此 ticks 形式 12.2.3加 */
static void ticks_to_sleep(uint32_t sleep_ticks) {
    uint32_t start_tick = ticks;
    /* 若间隔的 ticks 数不够便让出 cpu */
    while (ticks - start_tick < sleep_ticks) {
        thread_yield();
    }
}

/* 以毫秒为单位的sleep  1秒= 1000毫秒 13.2.3加 */
void mtime_sleep(uint32_t m_seconds) {
    uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
    ASSERT(sleep_ticks > 0);
    ticks_to_sleep(sleep_ticks);
}

