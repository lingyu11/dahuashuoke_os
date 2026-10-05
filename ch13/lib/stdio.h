#ifndef _STDIO_H
#define _STDIO_H

#include "stdint.h"
#include "string.h"

typedef char* va_list;

// 函数声明
uint32_t vsprintf(char* str, const char* format, va_list ap);
uint32_t sprintf(char* buf, const char* format, ...);
uint32_t printf(const char* format, ...);

#endif
