#include "timer.h"
#include "io.h"
#include "print.h"

#define PIT_CONTROL_PORT 0x43       //控制字寄存器端口


#define CONTRER0_PORT 0x40          //计数器0端口
#define COUNTER0_NO  0              //00  表选控制计数器0
#define READ_WRITE_LATCH 3          //11  先读写低字节 再读写高字节
#define COUNTER_MODE 2              //010 工作方式2


#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY

/* 设置控制寄存器、初始化计数器0的初始值 */
static void frequency_set (uint8_t counter_port, uint8_t counter_no, uint8_t rw1,uint8_t counter_mode, uint16_t counter_value) {

  outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rw1 << 4 | counter_mode << 1));
  outb (counter_port, (uint8_t)counter_value);     //先写入counter_value的低8位
  outb (counter_port, (uint8_t)counter_value >> 8);//再写入counter_value的高8位
}

/*初始化PIT8253*/
void timer_init() {
  put_str("timer_init start\n");
  //设置8253
  frequency_set(CONTRER0_PORT,COUNTER0_NO,READ_WRITE_LATCH,COUNTER_MODE,COUNTER0_VALUE);
  put_str("timer_init done\n");
}
