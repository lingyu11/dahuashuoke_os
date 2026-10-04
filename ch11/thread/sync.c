#include "sync.h"
#include "interrupt.h"     
#include "debug.h"         
#include "thread.h" 

/* 初始化信号量 */
void sema_init(struct semaphore* psema, uint8_t value) {
    psema->value = value;
    list_init(&psema->waiters);
}

/* 初始化锁 */
void lock_init(struct lock* plock) {
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);  // 信号量初值为1
}


//信号量操作
/* 信号量down操作，信号量减一 */
void sema_down(struct semaphore* psema) {
    enum intr_status old_status = intr_disable();
    while (psema->value == 0) {  // 值为0时阻塞
        ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);  // 阻塞当前线程
    }
    psema->value--;
    ASSERT(psema->value == 0);
    intr_set_status(old_status);
}

/* 信号量up操作，信号量加一*/
void sema_up(struct semaphore* psema) {
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters)) {
        struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);  // 唤醒等待线程
    }
    psema->value++;
    ASSERT(psema->value == 1);
    intr_set_status(old_status);
}

//锁的获取与释放：
/* 获取锁 */
void lock_acquire(struct lock* plock) {
    if (plock->holder != running_thread()) {  // 避免重复申请
        sema_down(&plock->semaphore);  // 信号量down
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    } else {
        plock->holder_repeat_nr++;
    }
}

/* 释放锁  */
void lock_release(struct lock* plock) {
    ASSERT(plock->holder == running_thread());
    if (plock->holder_repeat_nr > 1) {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    plock->holder = NULL;  // 先置空持有者
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);  // 信号量up
}
