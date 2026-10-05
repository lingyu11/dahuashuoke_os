#ifndef _FS_FILE_H
#define _FS_FILE_H

#include "fs.h"

struct partition;
struct dir;

extern struct file file_table[MAX_FILE_OPEN];

int32_t  get_free_slot_in_global(void);
int32_t  pcb_fd_install(int32_t globa_fd_idx);
int32_t  inode_bitmap_alloc(struct partition* part);
int32_t  block_bitmap_alloc(struct partition* part);
void     bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp);
int32_t  file_create(struct dir* parent_dir, char* filename, uint8_t flag);
int32_t  file_open(uint32_t inode_no, uint8_t flag);
int32_t  file_close(struct file* file);
int32_t  path_depth_cnt(char *pathname);

#endif
