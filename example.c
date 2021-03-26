#include <stdio.h>
#include "rax.h"

int testRax()
{
    //创建基数树
    rax *rt = raxNew();
    raxShow(rt);
    printf("\n\n");

    //插入key
    int ret;
    void *data;
    raxInsert(rt, (unsigned char *)"my", 2, "value", NULL);
    if (raxInsert(rt, (unsigned char *)"mykey", 5, "value", NULL))
    {
        printf("insert rax mykey success\n");
    }
    raxShow(rt);
    printf("\n\n");

    //覆盖插入
    if (raxInsert(rt, (unsigned char *)"mykey", 5, "other value", data) == 0)
    {
        printf("overwrite rax mykey success, %p\n", data);
    }

    if (raxTryInsert(rt, (unsigned char *)"mykey", 5, "try insert", NULL) == 0)
    {
        printf("key exist\n");
    }
    raxShow(rt);
    printf("\n\n");

    raxInsert(rt, (unsigned char *)"mykey1", 6, "value1", NULL);
    raxShow(rt);
    printf("\n\n");

    //查询
    data = raxFind(rt, (unsigned char *)"mykey", 5);
    if (data == raxNotFound)
    {
        printf("mykey not exist\n");
        return 0;
    }

    printf("Key value is %p\n", data);

    //迭代
    raxIterator iter;
    raxStart(&iter, rt); // Note that 'rt' is the radix tree pointer.
    raxSeek(&iter, ">=", (unsigned char *)"my", 2);
    while (raxNext(&iter))
    {
        if (raxCompare(&iter, ">", (unsigned char *)"mykey", 5))
            break;
        printf("Key: %.*s\n", (int)iter.key_len, (char *)iter.key);
    }
    raxStop(&iter);

    //删除
    if (raxRemove(rt, (unsigned char *)"mykey1", 6, &data))
    {
        printf("remove success\n");
        printf("remove value is %p\n", data);
    }

    if (raxRemove(rt, (unsigned char *)"mykey2", 6, &data) == 0)
    {
        printf("remove fail\n");
    }

    raxShow(rt);
    return 0;
}

#define raxPadding(nodesize) ((sizeof(void *) - ((nodesize + 4) % sizeof(void *))) & (sizeof(void *) - 1))
#define raxPadding2(nodesize) (sizeof(void *) - ((nodesize + sizeof(void *)) % sizeof(void *)))
int main()
{
    //testRax();
    printf("%d", sizeof(void *));
    for (int i = 0; i <= 16; i++)
    {
        printf("size = %d, padding = %d\n", i, raxPadding(i));
    }
    return 0;
}