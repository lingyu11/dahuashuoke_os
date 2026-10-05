#include "debug.h"
#include "print.h"
#include "interrupt.h"

/* 打印错误信息并挂起程序 */
void panic_spin(char* filename, int line, const char* func, const char* condition) {
    intr_disable();  // 关中断确保输出不被干扰

    put_str("\n\n!!!!!! error !!!!\n");
    put_str("filename:"); put_str(filename);         put_str("\n");
    put_str("line:0x");   put_int(line);             put_str("\n");
    put_str("function:"); put_str((char*)func);      put_str("\n");
    put_str("condition:");put_str((char*)condition); put_str("\n");

    while(1);  // 死循环挂起程序
}