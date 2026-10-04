#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "string.h"
#include "debug.h"

// area1 数据准备
#define PG_SIZE 4096                // 一页4K
#define MEM_BITMAP_BASE 0xc009a000  // 位图基地址
#define K_HEAP_START 0xc0100000     // 内核堆起始地址1M
#define MEM_SIZE 0X2000000          // 内存大小32M

/* 物理内存池结构 */
struct pool {
    struct bitmap pool_bitmap;  // 物理内存位图
    uint32_t phy_addr_start;    // 物理内存起始地址
    uint32_t pool_size;         // 内存池容量
};

/* 全局内存池实例 */
struct pool kernel_pool, user_pool;    // 内核物理内存池 用户内存池
struct virtual_addr kernel_vaddr_pool; // 此结构用于给内核分配虚拟地址

/* 地址转换宏 */
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)  // 页目录索引
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)  // 页表索引

/* 获取虚拟地址对应的页目录项指针*/
static uint32_t* pde_ptr(uint32_t vaddr) {
    //0xfffff000是特殊地址 利用页目录表1023项的映射页目录表自身
    uint32_t* pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
    return pde;
}

/* 获取虚拟地址对应的页表项指针*/
static uint32_t* pte_ptr(uint32_t vaddr) {
    uint32_t* pte=(uint32_t*)(0xffc00000+( (vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

// area2 内存池初始化，建立完整的内存管理框架，包括物理内存划分和虚拟地址空间设置

/* 内存池初始化 */
static void mem_pool_init(uint32_t all_mem) {
    put_str("mem_pool_init start\n");

    // 计算已使用内存（低端1MB+已分配页表空间）
    uint32_t page_table_size = PG_SIZE * 256;        // 加起来256个页框,1M
    uint32_t used_mem = page_table_size + 0x100000;  // 低1M加1M的页目录表和页表占用的空间
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;    // 可分配页

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
    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;     // 位图字节数
    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;   

    // 初始化用户物理内存池
    user_pool.phy_addr_start = up_start;
    user_pool.pool_size = user_free_pages * PG_SIZE;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;       
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length); 

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
        // 计算获得分配的虚拟地址起始地址
        vaddr_start = kernel_vaddr_pool.vaddr_start + bit_idx_start * PG_SIZE;
        put_int(kernel_vaddr_pool.vaddr_start);
        put_int(bit_idx_start);
        put_str("\n");
    } else {
        // 用户内存池暂不实现
        return NULL;
    }
    return (void*)vaddr_start;
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
