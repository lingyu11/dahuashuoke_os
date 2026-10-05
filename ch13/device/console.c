#include "console.h"
#include "print.h"
#include "stdint.h"
#include "sync.h"
#include "thread.h"

static struct lock console_lock;  // 全局终端锁

/* 初始化终端锁 */
void console_init() {
    lock_init(&console_lock);
}

/* 获取终端锁 */
void console_acquire() {
    lock_acquire(&console_lock);
}

/* 释放终端锁 */
void console_release() {
    lock_release(&console_lock);
}

/* 终端安全输出字符串 */
void console_put_str(char* str) {
    console_acquire();    // 加锁
    put_str(str);         // 调用底层输出
    console_release();    // 解锁
}

/* 终端安全输出字符 */
void console_put_char(uint8_t char_asci) {
    console_acquire();
    put_char(char_asci);
    console_release();
}

/* 终端安全输出十六进制整数 */
void console_put_int(uint32_t num) {
    console_acquire();
    put_int(num);
    console_release();
}
