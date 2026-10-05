#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

/* 环形队列 */
struct ioqueue {
// 生产者消费者问题
    struct lock lock;
/* 生产者，缓冲区不满时就继续往里面放数据，否则就睡眠，此项记录哪个生产者在此缓冲区上睡眠 */
    struct task_struct* producer;
/* 消费者，缓冲区不空时就继续从里面拿数据，否则就睡眠，此项记录哪个消费者在此缓冲区上睡眠 */
    struct task_struct* consumer;
    char buf[bufsize];           // 缓冲区大小
    int32_t head;                // 队首，数据往队首处写入
    int32_t tail;                // 队尾，数据从队尾处读出
};

/* 函数声明 */
void ioqueue_init(struct ioqueue* ioq);          // 初始化队列
bool ioq_full(struct ioqueue* ioq);              // 判断队列是否满
bool ioq_empty(struct ioqueue* ioq);             // 判断队列是否空（注：原为static，现改为公共接口）
char ioq_getchar(struct ioqueue* ioq);           // 从队列获取字符
void ioq_putchar(struct ioqueue* ioq, char byte); // 向队列写入字符
bool ioq_empty(struct ioqueue* ioq);


#endif
