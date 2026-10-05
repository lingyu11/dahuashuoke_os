#ifndef __FS_FS_H
#define __FS_FS_H

#include "stdint.h"
#include "list.h" 

#define MAX_FILES_PER_PART 4096   // 每个分区所支持最大创建的文件数
#define BITS_PER_SECTOR 4096      // 每扇区的位数
#define SECTOR_SIZE 512           // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE    // 块字节大小

/* 前向声明，防止循环依赖 */
struct inode;
struct dir;

/* 文件结构 */
struct file {
    uint32_t fd_pos;      // 当前文件操作偏移
    uint32_t fd_flag;     // 文件打开标志
    struct inode* fd_inode; // 指向inode
};

/* 标准文件描述符 */
enum std_fd {
    stdin_no,   // 0: 标准输入
    stdout_no,  // 1: 标准输出
    stderr_no   // 2: 标准错误
};

/* 位图类型 */
enum bitmap_type {
    INODE_BITMAP, // inode位图
    BLOCK_BITMAP  // 块位图
};

#define MAX_FILE_OPEN 32 // 系统最大打开文件数

#define MAX_PATH_LEN 512      // 路径最大长度

/* 文件类型 */
enum file_types {
    FT_UNKNOWN,     // 不支持的文件类型
    FT_REGULAR,     // 普通文件
    FT_DIRECTORY    // 目录
};

/* 打开文件的选项 */
enum oflags {
    O_RDONLY,       // 只读
    O_WRONLY,       // 只写
    O_RDWR,         // 读写
    O_CREAT = 4     // 创建
};

/* 用来记录查找文件过程中已找到的上级路径，
 * 也就是查找文件过程中“走过的地方” */
struct path_search_record {
    char searched_path[MAX_PATH_LEN];      // 查找过程中的父路径
    struct dir* parent_dir;      // 文件或目录所在的直接父目录
    enum file_types file_type;
    // 找到的是普通文件，还是目录，找不到将为未知类型(FT_UNKNOWN)
};

void filesys_init(void);
int32_t sys_open(const char* pathname, uint8_t flags);
int32_t sys_close(int32_t fd);

#endif
