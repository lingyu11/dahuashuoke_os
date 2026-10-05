#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "string.h"
#include "debug.h"
#include "sync.h"
#include "thread.h" 
#include "global.h" 

static void page_table_add(void* _vaddr, void* _page_phyaddr);
static void* palloc(struct pool* m_pool);
static void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);


/* 物理内存池结构 */
struct pool {
    struct bitmap pool_bitmap;  // 物理内存位图
    struct lock lock;                  // 申请内存时互斥
    uint32_t phy_addr_start;    // 物理内存起始地址
    uint32_t pool_size;         // 内存池容量
};
/* 内存仓库 */
struct arena {
    struct mem_block_desc* desc;    // 此 arena 关联的 mem_block_desc
    /* large 为 ture 时，cnt 表示的是页框数。否则 cnt 表示空闲 mem_block 数量 */
    uint32_t cnt;
    bool large;
};

struct mem_block_desc k_block_descs[DESC_CNT];      // 内核内存块描述符数组
/* 全局内存池实例 */
struct pool kernel_pool, user_pool;  // 内核物理内存池 用户内存池
struct virtual_addr kernel_vaddr_pool;    // 此结构用于给内核分配虚拟地址

/* 地址转换宏 */
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)  // 页目录索引
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)  // 页表索引

/* 为malloc做准备,初始化内存快数组 */
void block_desc_init(struct mem_block_desc* desc_array) {
    uint16_t desc_idx, block_size = 16;

    /* 初始化每个 mem_block_desc 描述符 */
    for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
        desc_array[desc_idx].block_size = block_size;
        /* 初始化 arena 中的内存块数量 */
        desc_array[desc_idx].blocks_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
        list_init(&desc_array[desc_idx].free_list);
        block_size *= 2;    // 更新为下一个规格内存块
    }
}

/* 获取虚拟地址对应的页目录项指针*/
static uint32_t* pde_ptr(uint32_t vaddr) {
    //0xfffff000是特殊地址 利用页目录表1023项的映射页目录表自身
    uint32_t* pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
    return pde;
}

/* 获取虚拟地址对应的页表项指针*/
static uint32_t* pte_ptr(uint32_t vaddr) {
    uint32_t* pte=(uint32_t*)(0xffc00000+( (vaddr & 0xffc00000)>>10) + PTE_IDX(vaddr) * 4);
    return pte;
}

// area2 内存池初始化，建立完整的内存管理框架，包括物理内存划分和虚拟地址空间设置

/* 内存池初始化 */
static void mem_pool_init(uint32_t all_mem) {
    put_str("mem_pool_init start\n");
    // 计算已使用内存（低端1MB+已分配页表空间）
    uint32_t page_table_size = PG_SIZE * 256;        //加起来256个页框,1M
    uint32_t used_mem = page_table_size + 0x100000;  // 低1M加1M的页目录表和页表占用的空间
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;    //可分配页
    
    // 计算分配空闲页给内核和用户,单位为页
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;
    
    // 计算位图长度（每页对应1位）,单位为字节 
    uint32_t kbm_length = kernel_free_pages / 8;  //余数对应页直接丢弃 不算在内
    uint32_t ubm_length = user_free_pages / 8;
  
    kernel_free_pages = kbm_length*8;
    user_free_pages = ubm_length*8;
  
    // 计算内存池起始地址
    uint32_t kp_start = used_mem;  // 内核物理内存池的起始地址 计算后为2M
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;  // 用户物理内存池起始地址
    
    // 初始化内核物理内存池
    kernel_pool.phy_addr_start = kp_start;
    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;     //位图字节数
    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE; 
    lock_init(&kernel_pool.lock);
    
    // 初始化用户物理内存池
    user_pool.phy_addr_start = up_start;
    user_pool.pool_size = user_free_pages * PG_SIZE;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;       
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length); 
    lock_init(&user_pool.lock);
    
    // 初始化位图
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);
    
    // 初始化内核虚拟地址池 匹配内核物理内存池大小
    kernel_vaddr_pool.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr_pool.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    kernel_vaddr_pool.vaddr_start = K_HEAP_START;
    
    // 初始化位图
    bitmap_init(&kernel_vaddr_pool.vaddr_bitmap);
    
    put_str("mem_pool_init done\n");
}

// 内存管理部分初始化入口
void mem_init() {
    put_str("mem_init start\n");
    //uint32_t mem_bytes_total = (*(uint32_t*)(0xb00)); //此地址存储了物理内存大小
    uint32_t mem_bytes_total=MEM_SIZE;
    mem_pool_init(mem_bytes_total);
    block_desc_init(k_block_descs);           //12.4.2加
    put_str("mem_init done\n");
}

// area3 页分配，核心部分，主要包括虚拟地址分配、物理页分配和页表映射三个步骤

/* 在虚拟地址池中分配pg_cnt个虚拟页  返回分配后的虚拟地址起始地址信息 */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if (pf == PF_KERNEL) {
        bit_idx_start = bitmap_scan(&kernel_vaddr_pool.vaddr_bitmap, pg_cnt); 
        if (bit_idx_start == -1) {
            return NULL;
        }
        while (cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr_pool.vaddr_bitmap, bit_idx_start + cnt, 1);
            cnt++;
        }
        //计算获得分配的虚拟地址其实地址
        vaddr_start = kernel_vaddr_pool.vaddr_start + bit_idx_start * PG_SIZE;
        //put_int(kernel_vaddr_pool.vaddr_start);
        //put_int(bit_idx_start);
        put_str("\n");
    } else {
        // 用户内存池
        struct task_struct* cur = running_thread();
        bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1) {
            return NULL;
        }

        while(cnt < pg_cnt) {
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }
        vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;

        /* (0xc0000000 - PG_SIZE)作为用户3级栈已经在start_process被分配 */
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }
    return (void*)vaddr_start;
}
  
  
/* 在用户空间中申请4k内存，并返回其虚拟地址 */
void* get_user_pages(uint32_t pg_cnt) {
    lock_acquire(&user_pool.lock);
    void* vaddr = malloc_page(PF_USER, pg_cnt);
    memset(vaddr, 0, pg_cnt * PG_SIZE);
    lock_release(&user_pool.lock);
    return vaddr;
}

/* 设置地址vaddr对应的用户或者内核虚拟地址池，分配一页物理内存，并进行关联，仅支持一页空间分配 */
void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    lock_acquire(&mem_pool->lock);

    /* 先将虚拟地址对应的位图置1 */
    struct task_struct* cur = running_thread();
    int32_t bit_idx = -1;

/* 若当前是用户进程申请用户内存，就修改用户进程自己的虚拟地址位图 */
    if (cur->pgdir != NULL && pf == PF_USER) {
        bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);

    } else if (cur->pgdir == NULL && pf == PF_KERNEL) {
/* 如果是内核线程申请内核内存，就修改kernel_vaddr_pool */
        bit_idx = (vaddr - kernel_vaddr_pool.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr_pool.vaddr_bitmap, bit_idx, 1);
    } else {
        PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
    }

    void* page_phyaddr = palloc(mem_pool);
    if (page_phyaddr == NULL) {
        return NULL;
    }
    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}

/* 得到虚拟地址映射到的物理地址 */
uint32_t addr_v2p(uint32_t vaddr) {
    uint32_t* pte = pte_ptr(vaddr);
/* (*pte)的值是页表所在的物理页框地址，
 * 去掉其低12位的页表项属性+虚拟地址vaddr的低12位 */
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}
  

/* 在物理内存池中分配1个物理页  返回分配的物理地址起始地址信息*/
static void* palloc(struct pool* m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1); //1个物理页
    if (bit_idx == -1) {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = m_pool->phy_addr_start + bit_idx * PG_SIZE;
    return (void*)page_phyaddr;
}

/* 设置页目录项、页表项，在页表中添加虚拟地址到物理地址的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
    uint32_t  vaddr = (uint32_t)_vaddr;
    uint32_t  page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);
    
    if (*pde & 0x00000001) {           // 页目录项对应的页表存在
        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    } else {                            // 创建新页表
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);     //设置页目录表项
        memset( (void*)((int)pte & 0xfffff000), 0, PG_SIZE );  // 清空新页表
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
   
}

/* 分配pg_cnt个页(包括物理页 虚拟地址页 映射关系设置)*/
static void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    // 分配虚拟地址
    void* vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL) {
        return NULL;
    }
    
    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t cnt = pg_cnt;
    struct pool* mem_pool = (pf == PF_KERNEL) ? &kernel_pool : &user_pool;//物理池指针
    
    // 为每个虚拟页分配物理页并建立映射
    while (cnt-- > 0) {
        void* page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL) {
                           // 分配失败应回滚已分配资源
          return NULL; 
        }
        page_table_add((void*)vaddr, page_phyaddr);
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}

/* 内核内存分配接口 malloc_page的简单封装 */
void* get_kernel_pages(uint32_t pg_cnt) {
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, pg_cnt * PG_SIZE);  // 清空分配的内存
    }
    return vaddr;
}

/* 返回arena中第idx个内存块的地址 */
static struct mem_block* arena2block(struct arena* a, uint32_t idx) {
    return (struct mem_block*) ((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

/* 返回内存块b所在的arena地址 */
static struct arena* block2arena(struct mem_block* b) {
    return (struct arena*) ((uint32_t)b & 0xfffff000);
}

/* 在堆中申请size个字节内存 */
void* sys_malloc(uint32_t size) {
    enum pool_flags PF;
    struct pool* mem_pool;
    uint32_t pool_size;
    struct mem_block_desc* descs;
    struct task_struct* cur_thread = running_thread();

    /* 判断用哪个内存池 */
    if (cur_thread->pgdir == NULL) {      // 若为内核线程
        PF = PF_KERNEL;
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = k_block_descs;
    } else {                              // 用户进程pcb中的pgdir会在为其分配页表时创建
        PF = PF_USER;
        pool_size = user_pool.pool_size;
        mem_pool = &user_pool;
        descs = cur_thread->u_block_desc;
    }

    /* 若申请的内存不在内存池容量范围内，则直接返回 NULL */
    if (!(size > 0 && size < pool_size)) {
        return NULL;
    }

    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);

    /* 超过最大内存块1024，就分配页框 */
    if (size > 1024) {
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);
        // 向上取整需要的页框数
        a = malloc_page(PF, page_cnt);

        if (a != NULL) {
            memset(a, 0, page_cnt * PG_SIZE);     // 将分配的内存清0
            /* 对于分配的大块页框，将desc置为NULL，cnt置为页框数，large置为true */
            a->desc = NULL;
            a->cnt = page_cnt;
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a + 1); // 跨过arena大小，把剩下的内存返回
        } else {
            lock_release(&mem_pool->lock);
            return NULL;
        }
    } else {    // 若申请的内存小于等于1024，可在各种规格的mem_block_desc中去适配
        uint8_t desc_idx;
        //从内存块描述符中匹配合适的内存块规格 从小往大后，找到后退出
        for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
            if (size <= descs[desc_idx].block_size) {
                break;
            }
        }

        // 若mem_block_desc的free_list中已经没有可用的mem_block，就创建新的arena提供 mem_block
        if (list_empty(&descs[desc_idx].free_list)) {
            a = malloc_page(PF, 1);       // 分配1页框作为 arena
            if (a == NULL) {
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(a, 0, PG_SIZE);

            // 对于分配的小块内存，将desc置为相应内存块描述符，cnt置为此arena可用的内存块数，large置为false
            a->desc = &descs[desc_idx];
            a->large = false;
            a->cnt = descs[desc_idx].blocks_per_arena;
            uint32_t block_idx;

            enum intr_status old_status = intr_disable();

            /* 开始将arena拆分成内存块，并添加到内存块描述符的free_list 中 */
            for (block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++) {
                b = arena2block(a, block_idx);
                ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
                list_append(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }

        /* 开始分配内存块 */
        b = elem2entry(struct mem_block, free_elem, list_pop(&(descs[desc_idx].free_list)) );
        memset(b, 0, descs[desc_idx].block_size);

        a = block2arena(b);   // 获取内存块 b 所在的 arena
        a->cnt--;             // 将此 arena 中的空闲内存块数减 1
        lock_release(&mem_pool->lock);
        return (void*)b;
    }
}

/* 将物理地址pg_phy_addr回收到物理内存池，即位图置0 */
static void pfree(uint32_t pg_phy_addr) {
    struct pool* mem_pool;
    uint32_t bit_idx = 0;
    if (pg_phy_addr >= user_pool.phy_addr_start) {      // 用户物理内存池
        mem_pool = &user_pool;
        bit_idx = (pg_phy_addr - user_pool.phy_addr_start) / PG_SIZE;
    } else {                                            // 内核物理内存池
        mem_pool = &kernel_pool;
        bit_idx = (pg_phy_addr - kernel_pool.phy_addr_start) / PG_SIZE;
    }
    bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);     // 将位图中该位清 0
}

/* 去掉页表中虚拟地址vaddr的映射，只去掉vaddr对应的pte */
static void page_table_pte_remove(uint32_t vaddr) {
    uint32_t* pte = pte_ptr(vaddr);
    *pte &= ~PG_P_1;                                    // 将页表项 pte 的 P 位置 0
    asm volatile ("invlpg %0"::"m" (vaddr):"memory");   // 更新 tlb
}

/* 在虚拟地址池中释放以_vaddr起始的连续pg_cnt个虚拟页地址，即位图置0 */
static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
    uint32_t bit_idx_start = 0, vaddr = (uint32_t)_vaddr, cnt = 0;

    if (pf == PF_KERNEL) {                              // 内核虚拟内存池
        bit_idx_start = (vaddr - kernel_vaddr_pool.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr_pool.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    } else {                                            // 用户虚拟内存池
        struct task_struct* cur_thread = running_thread();
        bit_idx_start = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt) {
            bitmap_set(&cur_thread->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
}

/* 释放以虚拟地址vaddr为起始的cnt个物理页框 */
static void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
    uint32_t pg_phy_addr;
    uint32_t vaddr = (int32_t)_vaddr, page_cnt = 0;
    ASSERT(pg_cnt >=1 && vaddr % PG_SIZE == 0);
    pg_phy_addr = addr_v2p(vaddr);  //获取虚拟地址vaddr对应的物理地址

  
    /* 确保待释放的物理内存存在低端1MB+ 4KB大小的页目录 +4KB大小的页表地址范围外 */
    ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= 0x102000);

    /* 判断pg_phy_addr属于用户物理内存池还是内核物理内存池 */
    if (pg_phy_addr >= user_pool.phy_addr_start) {      // 位于 user_pool 内存池
        vaddr -= PG_SIZE;
        while (page_cnt < pg_cnt) {
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);
            /* 确保物理地址属于用户物理内存池 */
            ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= user_pool.phy_addr_start);
            pfree(pg_phy_addr);          //先将对应的物理页框归还到内存池
            page_table_pte_remove(vaddr);//再从页表中清除此虚拟地址所在的页表项pte
            page_cnt++;
        }
        vaddr_remove(pf, _vaddr, pg_cnt);   //清空虚拟地址的位图中的相应位
    } else {                                            // 位于 kernel_pool 内存池
        vaddr -= PG_SIZE;
        while (page_cnt < pg_cnt) {
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);
            /* 确保待释放的物理内存只属于内核物理内存池 */
            ASSERT((pg_phy_addr % PG_SIZE) == 0 && \
                   pg_phy_addr >= kernel_pool.phy_addr_start && \
                   pg_phy_addr < user_pool.phy_addr_start);

            pfree(pg_phy_addr);          //先将对应的物理页框归还到内存池
            page_table_pte_remove(vaddr);//再从页表中清除此虚拟地址所在的页表项 pte
            page_cnt++;
        }
        vaddr_remove(pf, _vaddr, pg_cnt);//清空虚拟地址的位图中的相应位
    }
}

/* 回收内存 ptr */
void sys_free(void* ptr) {
    ASSERT(ptr != NULL);
    if (ptr != NULL) {
        enum pool_flags PF;
        struct pool* mem_pool;

        /* 判断是线程，还是进程 */
        if (running_thread()->pgdir == NULL) {
            ASSERT((uint32_t)ptr >= K_HEAP_START);
            PF = PF_KERNEL;
            mem_pool = &kernel_pool;
        } else {
            PF = PF_USER;
            mem_pool = &user_pool;
        }

        lock_acquire(&mem_pool->lock);
        struct mem_block* b = ptr;
        struct arena* a = block2arena(b);                   //mem_block转换成arena, 获取元信息

        ASSERT(a->large == 0 || a->large == 1);
        if (a->desc == NULL && a->large == true) {          //大于1024的内存
            mfree_page(PF, a, a->cnt);
        } else {                                            //小于等于 1024 的内存块
            list_append(&a->desc->free_list, &b->free_elem);//先将内存块回收到 free_list

            //再判断此arena中的内存块是否都是空闲，如果是就释放 arena */
            if (++a->cnt == a->desc->blocks_per_arena) {
                uint32_t block_idx;
                for (block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
                    struct mem_block* b = arena2block(a, block_idx);
                    ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
                    list_remove(&b->free_elem);
                }
                mfree_page(PF, a, 1);
            }
        }
        lock_release(&mem_pool->lock);
    }
}

