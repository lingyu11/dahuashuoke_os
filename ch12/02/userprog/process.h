#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H
#include "thread.h"
#include "stdint.h"

/* 创建用户进程 */
void process_execute(void* filename, char* name);
/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(struct task_struct* user_prog);
/* 构建用户进程初始上下文信息 */
void start_process(void* filename_);
/* 创建页目录表，返回页目录的虚拟地址 */
uint32_t* create_page_dir(void);
void process_activate(struct task_struct* p_thread);
void page_dir_activate(struct task_struct* p_thread);

#endif
