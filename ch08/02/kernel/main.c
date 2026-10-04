#include "print.h"
#include "init.h"
#include "memory.h"

int main(void) {
    put_str("I am kernel\n");
    init_all();

    // 测试内存分配
    void* addr = get_kernel_pages(3);
    put_str("get_kernel_page start vaddr is ");
    put_int((uint32_t)addr);
    put_str("\n");

    while(1);
    return 0;
}