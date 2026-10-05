#include "print.h"
#include "init.h"
#include "thread.h"
#include "process.h"
#include "console.h"
#include "interrupt.h"
#include "stdio.h"
#include "syscall-init.h"
#include "syscall.h" 
#include "fs.h" 

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int main(void) {
    put_str("I am kernel\n");
    init_all();
    intr_enable();
    process_execute(u_prog_a, "u_prog_a");
    process_execute(u_prog_b, "u_prog_b");
    thread_start("k_thread_a", 31, k_thread_a, "I am thread_a");
    thread_start("k_thread_b", 31, k_thread_b, "I am thread_b");
    
    //创建
    
    sys_open("/file1", O_CREAT);
    //while(1);
    //return 0;
    

    //打开
    
    uint32_t fd = sys_open("/file1", O_RDONLY);
    printf("fd:%d\n", fd);
    sys_close(fd);
    printf("%d closed now\n", fd);
    while(1);
    return 0;
    
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {
    void* addr1 = sys_malloc(256);
    void* addr2 = sys_malloc(255);
    void* addr3 = sys_malloc(254);
    //console_put_char('\n');

    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {
    // 代码同 k_thread_a
    void* addr1 = sys_malloc(256);
    void* addr2 = sys_malloc(255);
    void* addr3 = sys_malloc(254);

    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
    void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);

    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1);
}

/* 测试用户进程 */
void u_prog_b(void) {
    // 代码同 u_prog_a
    void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);

    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1);
}
