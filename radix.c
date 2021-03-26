#include "radix.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define radixPadding(nodesize) ((sizeof(void *) - ((nodesize + 4) % sizeof(void *))) & (sizeof(void *) - 1))
#define radixNodeLen(n) (sizeof(radixNode) + radixPadding(n))

//创建节点，分配孩子字符空间
radixNode *radixNewNode(size_t children)
{

    radixNode *node = radix_malloc(radixNodeLen(children));
    if (node == NULL)
        return NULL;
    node->iskey = 0;
    node->isnull = 0;
    node->iscompr = 0;
    node->size = children;
    node->children = NULL;
    if (children)
        node->children = radix_malloc(sizeof(radixNode *) * children);
    else
        node->children = NULL;
    return node;
}

//创建基数树
radix *radixNew(void)
{
    radix *tree = radix_malloc(sizeof(radix));
    if (tree == NULL)
        return NULL;
    tree->root = radixNewNode(0);
    if (tree->root == NULL)
    {
        radix_free(tree);
        return NULL;
    }
    tree->numele = 0;
    tree->numnodes = 1;
    return tree;
}

//查询node
radixNode *radixFind(radix *r, unsigned char *str, size_t len, size_t *suffix_len, size_t *split_pos)
{
    radixNode *node = r->root;
    size_t i = 0;
    size_t j = 0;

    // 遍历用户插入的字符串，前缀匹配
    while (node->size && i < len)
    {
        unsigned char *v = node->data;

        if (node->iscompr) // 压缩节点
        {
            for (j = 0; j < node->size && i < len; j++, i++)
            {
                if (v[j] != str[i])
                    break;
            }
            // 部分匹配，则终止walk，因为此节点是压缩节点，如果部分匹配，则没有孩子节点，所以停止
            if (j != node->size)
                break;
            // j = node->size, 压缩节点str是插入str的前缀，继续下一个节点的后续匹配
        }
        else // 非压缩节点，从孩子中寻找匹配的字符才能继续
        {
            for (j = 0; j < node->size; j++)
            {
                // 匹配节点第j个字符，也即此节点的第j个孩子
                if (v[j] == str[i])
                    break;
            }
            if (j == node->size) // 遍历了所有孩子，没找到共同部分，彼此无前缀，则终止walk
                break;
            //j !=  node->size, 配置第j个孩子，继续匹配后续字符。
            i++;
        }

        if (node->iscompr && j == node->size)
        {
            j = 0;
        }

        node = node->children[j];
    }
    /* 到这里4种情况
     * 1. iscompr = 1, 不匹配或者部分匹配，此时需要后续拆分节点
     * 2. iscompr = 0, 没有匹配字符，需要后续插入
     * 3. node遍历结束
     * 4. 字符串遍历结束，此时也需要拆分节点
     */
    *suffix_len = i;
    *split_pos = j;
    return node;
}

//插入孩子
radixNode *radixInsertChild(radixNode *parent, char ch, radixNode **child)
{
    if (parent == NULL || ch == '\0')
        return parent;
    if (parent->iscompr)
    {
        if (radixPadding(parent->size))
        {
            parent->data[parent->size] = ch;
        }
        parent->size++;
        return parent;
    }

    //寻找插入位置
    int i;
    for (i = 0; i < parent->size; i++)
    {
        if (parent->data[i] > ch)
            break;
    }

    //创建孩子
    *child = radixNewNode(0);
    if (*child == NULL)
        return parent;

    //分配字符
    if (!radixPadding(parent->size)) //不需要重新分配
    {
        radixNode *new_node = radix_realloc(parent, radixNodeLen(parent->size + 1));
        if (new_node == NULL)
            return NULL;
        parent = new_node;
    }

    //分配指针
    radixNode **new_list = radix_realloc(parent->children, (parent->size + 1) * sizeof(radixNode *));
    if (new_list == NULL)
    {
        radix_free(*child);
        return parent;
    }
    parent->children = new_list;

    for (int j = parent->size; j > i; j--)
    {
        parent->data[j] = parent->data[j - 1];
        parent->children[j] = parent->children[j - 1];
    }
    parent->data[i] = ch;
    parent->children[i] = child;
    return parent;
}

//插入key
int *radixInsert(radix *r, unsigned char *str, size_t len)
{
    if (r == NULL)
        return -1; //无效树
    if (len == 0 || str == NULL)
        return 0; //无效插入
                  //寻找插入位置
    radixNode *pos;
    size_t suffix, split;
    pos = radixFind(r, str, len, &suffix, &split);
    //TODO
    //插入孩子
    return 1;
}
//删除节点
//查找节点