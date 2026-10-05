#ifndef __FS_DIR_H
#define __FS_DIR_H

#include "inode.h"
#include "super_block.h"
#include "stdio-kernel.h"
#include "string.h"
#include "debug.h"
#include "file.h"
#include "stdint.h"
#include "global.h"

#define MAX_FILE_NAME_LEN 16  // 最大文件名长度

/* 目录结构(内存中使用)*/
struct dir {
    struct inode* inode;      // 指向inode
    uint32_t dir_pos;         // 目录内偏移
    uint8_t dir_buf[512];     // 目录数据缓存
};

/* 目录项结构(磁盘上存储)*/
struct dir_entry {
    char filename[MAX_FILE_NAME_LEN]; // 文件名
    uint32_t i_no;                    // 对应inode编号
    enum file_types f_type;           // 文件类型
};

/* 外部全局变量声明 */
extern struct partition* cur_part;
extern struct dir root_dir;  // 根目录

/* 打开根目录 */
void open_root_dir(struct partition *part);

/* 在分区part上打开i结点为inode_no的目录并返回目录指针 */
struct dir *dir_open(struct partition *part, uint32_t inode_no);

/* 在part分区内的pdir目录内寻找名为name的文件或目录,
 * 找到后返回true，并将其目录项存入dir_e,否则返回false */
bool search_dir_entry(struct partition *part, struct dir *pdir, const char *name, struct dir_entry *dir_e);

/* 关闭目录 */
void dir_close(struct dir *dir);

/* 在内存中初始化目录项p_de */
void create_dir_entry(char *filename, uint32_t inode_no, uint8_t file_type, struct dir_entry *p_de);

/* 将目录项p_de写入父目录parent_dir中,io_buf由主调函数提供 */
bool sync_dir_entry(struct dir *parent_dir, struct dir_entry *p_de, void *io_buf);

#endif  // __FS_DIR_H
