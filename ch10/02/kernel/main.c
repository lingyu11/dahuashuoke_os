#include "print.h"
#include "init.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"


void k_thread_a(void*);
void k_thread_b(void*);

int main(void) {
    put_str("I am kernel\n");
    init_all();
    //thread_start("k_thread_a", 31, k_thread_a, "argA");  //本节注释
    //thread_start("k_thread_b", 8, k_thread_b, "argB");   //本节注释
    intr_enable();
    while(1); //{  //本节注释
        //console_put_str("Main");  //本节注释
    //};  //本节注释
    return 0;
}

void k_thread_a(void* arg) {
    char* para = arg;
    while(1) {
        console_put_str(para);
    }
}

void k_thread_b(void* arg) {
    char* para = arg;
    while(1) {
        console_put_str(para);
    }
}