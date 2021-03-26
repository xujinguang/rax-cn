#ifndef RADIX_H
#define RADIX_H

#include <stdint.h>

typedef struct radixNode
{
    uint32_t iskey : 1;          /* Does this node contain a key? */
    uint32_t isnull : 1;         /* Associated value is NULL (don't store it). */
    uint32_t iscompr : 1;        /* Node is compressed. */
    uint32_t size : 29;          //data长度
    struct radixNode **children; //孩子指针
    void *value;                 //关联的值
    unsigned char data[];        //字符串
} radixNode;

typedef struct radix
{
    radixNode *root;   //基数树根
    uint64_t numele;   //key的个数
    uint64_t numnodes; //树节点个数
} radix;

#define radix_malloc malloc
#define radix_realloc realloc
#define radix_free free

radix *radixNew(void);
#endif