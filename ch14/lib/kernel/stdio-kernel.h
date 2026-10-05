#ifndef __PRINTK_H__
#define __PRINTK_H__

#define va_start(ap, v)  ap = (va_list)&v      // 把ap指向第一个固定参数 v
#define va_arg(ap, t)    *((t*)(ap += 4))      // ap指向下一个参数并返回其值
#define va_end(ap)       ap = NULL             // 清除 ap


// 打印函数声明
void printk(const char* format, ...);

#endif
