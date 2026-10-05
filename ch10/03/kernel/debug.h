#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H
#define NULL ((void*)0)

void panic_spin(char* filename, int line, const char* func, const char* condition);

/* 定义ASSERT宏 */
#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else
    #define ASSERT(CONDITION) \
        if (CONDITION) {} else { \
            panic_spin(__FILE__, __LINE__, __func__, #CONDITION); \
        }
#endif

#endif