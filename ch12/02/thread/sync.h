#ifndef __SYNC_H
#define __SYNC_H

#include "list.h"          // 包含链表操作
#include "thread.h"        // 包含线程结构定义
#include "stdint.h"        // 包含基本数据类型

/* 信号量结构 */
struct semaphore {
    uint8_t value;         // 信号量值
    struct list waiters;   // 等待队列
};

/* 锁结构 */
struct lock {
    struct task_struct* holder;        // 锁持有者
    struct semaphore semaphore;       // 二元信号量
    uint32_t holder_repeat_nr;        // 重复申请次数
};


/* ==================== 信号量操作 ==================== */


void sema_init(struct semaphore* psema, uint8_t value);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);

/* ==================== 锁操作 ==================== */


void lock_init(struct lock* plock);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif

