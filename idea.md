花了两天时间阅读了一下rax实现的源码，感觉不优雅。
问题出在数据结构的定义上，一味的节省空间，和内存的方便分配，把字符串，树指针和数据全部存储在rax.data中
然后实现几个辅助函数来获取这些分量，这个还不是核心问题；重点是插入和合并的时候，对内存的来回移动让代码的
可阅读性大大降低。作者在指引文档中也特别强调实现时候的困难性，说明这个设计本身有不合理性，导致它的实现变
的复杂以及难以调试。

将原有的结构拆分如下成员
```c
typedef struct radixNode
{
    uint32_t iskey : 1;   /* Does this node contain a key? */
    uint32_t isnull : 1;  /* Associated value is NULL (don't store it). */
    uint32_t iscompr : 1; /* Node is compressed. */
    uint32_t size : 29;   //兄弟个数
    struct radixNode *brothers; //兄弟指针
    struct radixNode *children; //孩子指针
    void *value; //关联的值
    char data[]; //字符串
} radix;
```

改变后的实现参考radix.c