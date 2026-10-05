#ifndef __USERPROG_TSS_H
#define __USERPROG_TSS_H
#include "thread.h"

/* 更新tss中esp0字段的值为pthread的0级线 */
void update_tss_esp(struct task_struct* pthread);
/* 在gdt中创建tss并重新加载gdt */
void tss_init(void);
#endif
