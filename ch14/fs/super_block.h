#ifndef _FS_SUPER_BLOCK_H
#define _FS_SUPER_BLOCK_H

#include "stdint.h" // 使用自定义的头文件

/* 扇区大小定义，用于计算下方的填充字节 */
#define SECTOR_SIZE 512

/* 超级块 */
struct super_block {
    uint32_t magic;                // 文件系统类型标识
    uint32_t sec_cnt;              // 本分区总扇区数
    uint32_t inode_cnt;            // 本分区inode数量
    uint32_t part_lba_base;        // 本分区起始LBA地址
    uint32_t block_bitmap_lba;     // 块位图起始扇区
    uint32_t block_bitmap_sects;   // 块位图占用扇区数
    uint32_t inode_bitmap_lba;     // inode位图起始扇区
    uint32_t inode_bitmap_sects;   // inode位图占用扇区数
    uint32_t inode_table_lba;      // inode表起始扇区
    uint32_t inode_table_sects;    // inode表占用扇区数
    uint32_t data_start_lba;       // 数据区起始扇区
    uint32_t root_inode_no;        // 根目录inode号
    uint32_t dir_entry_size;       // 目录项大小
    uint8_t pad[460];              // 填充至512字节 (12个uint32_t占48字节 + 460 = 512)
} __attribute__((packed));

#endif
