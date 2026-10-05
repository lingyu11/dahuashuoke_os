#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "ide.h"
#include "inode.h"
#include "debug.h"
#include "super_block.h"
#include "string.h"
#include "interrupt.h"
#include "stdbool.h"      
#include "list.h"        

/* inode结构 */
struct inode {
    uint32_t i_no;              // inode编号
    uint32_t i_size;            // 文件时，i_size指文件大小,若是目录，指目录项大小之和
    uint32_t i_open_cnts;       // 文件被打开次数
    bool write_deny;            // 写保护标志
    uint32_t i_sectors[13];     // 总共支持128+12=140块(扇区)[0-11]是直接块 [12]存储一级间接块指针
    struct list_elem inode_tag; // 用于inode链表(此链表为已打开的inode队列，可视为内存缓冲区)
};


/* 将inode写入到分区part */
void inode_sync(struct partition *part, struct inode *inode, void *io_buf);

/* 根据i结点号返回相应的i结点 */
struct inode *inode_open(struct partition *part, uint32_t inode_no);

/* 关闭inode或减少inode的打开数 */
void inode_close(struct inode *inode);

/* 初始化new_inode */
void inode_init(uint32_t inode_no, struct inode *new_inode);

#endif  // __FS_INODE_H
