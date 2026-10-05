#ifndef __STRING_H__
#define __STRING_H__

#include "global.h"

// 内存操作函数
void memset(void* dst_, uint8_t value, uint32_t size);       //内存设置 
void memcpy(void* dst_, const void* src_, uint32_t size);    //实现内存数据拷贝
int  memcmp(const void* a_, const void* b_, uint32_t size);  //比较两段内存数据的内容

// 字符串操作函数
char*    strcpy(char* dst_, const char* src_);       //复制字符串
uint32_t strlen(const char* str);                    //计算字符串长度
int8_t strcmp(const char* a, const char* b);         //比较两个字符串的内容
char*  strchr(const char* str, const uint8_t ch);    //定位字符在字符串中的首次出现位置
char*  strrchr(const char* str, const uint8_t ch);   //从字符串末尾开始查找字符
char*  strcat(char* dst_, const char* src_);         //实现字符串拼接
uint32_t strchrs(const char* str, uint8_t ch);       //统计字符在字符串中出现的次数

#endif // __STRING_H__
