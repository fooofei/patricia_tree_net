
#ifndef PATRICIA_TREE_H
#define PATRICIA_TREE_H

#include <stdint.h>
#include <stdio.h>

#ifdef WIN32
#include <In6addr.h>
#include <Windows.h>
#else

#include <netinet/in.h>

#endif

#include "prefix.h"

enum {
    PATRICIA_MAXBITS = sizeof(struct in6_addr) * 8,
};
#define PATRICIA_MAXBITS PATRICIA_MAXBITS

struct patnode {
    struct patnode* next; // used for list
    struct patnode* left;
    struct patnode* right;
    struct patnode* parent;
    uint8_t maskbit; /* 不一定等于 prefix->mask  可能是其他 */
    struct prefix* prefix;
    const char* prefix_string;
    struct prefix _a;
};

/* 这颗树的最大深度是 max_maskbit 所以树中的递归不担忧栈溢出 */
struct patree {
    struct patnode* root;
    uint8_t addr_family;
    uint32_t max_maskbit;
    int node_cnt;
    int glue_cnt;
    struct patnode* glue_list;
};

void patnode_fprintf(FILE* f, const struct patnode* node);

int patree_init(struct patree* tree, uint8_t addr_family);

/* Insert prefix to tree.
    Return the new node contain the prefix or old node contain the prefix.
    The prefix is almost same, e.g. will find 10.0.0.0/9 for 10.1.0.0/9.
    同 1 个 net addr （相同子网掩码）不会学习两遍。

Lookup string the format of IP/MASK.
node contains prefix
*/
struct patnode* patree_lookup(struct patree* tree, struct patnode* node);

// Format IP/Mask to struct patnode
// return node  for success,
// return NULL for fail
struct patnode* patnode_format(struct patnode* node, const char* str);

// 返回自身
struct patnode* patree_search_exact(const struct patree* tree, const char* p);

// 返回结果优先级：可能是自身的、按照子网掩码计算最长匹配的
struct patnode* patree_search_best(const struct patree* tree, const char* p);

/* 左旋转 90 度打印树. */
void patree_fprintf(FILE* f, const struct patree* tree);

void patree_table(FILE* f, const struct patree* tree);

void patree_term(struct patree* tree);

#ifdef DEBUG
#define ASSERT(expr)  assert((expr))
#else
#define ASSERT(expr)  ((void)0)
#endif

#endif /* _PATRICIA_H */
