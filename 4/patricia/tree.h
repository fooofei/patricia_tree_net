
/*
  Only support IPv4
*/

#ifndef PATRICIA_TREE_H
#define PATRICIA_TREE_H

#include <stdint.h>
#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
#include <In6addr.h>
#else
#include <netinet/in.h>
#endif

#include "prefix.h"



enum {
    PATRICIA_MAXBITS = sizeof(struct in6_addr) * 8,
};
#define PATRICIA_MAXBITS	PATRICIA_MAXBITS

struct patnode
{
    struct patnode* left;
    struct patnode* right;
    struct patnode* parent;
    uint8_t maskbit;  /* 不一定等于 prefix->mask  可能是其他 */
    struct prefix* prefix;
    const char* prefix_string;
    struct prefix _a;
};

/* 这颗树的最大深度是 32 所以树中的递归不担忧栈溢出 */
struct patree
{
    struct patnode* root;
    uint32_t maxbits;/* for IP, 32 bit addresses  难道这个是所有结点的最大 mask ？ */
    int node_cnt;
    int glue_cnt;
};




void patnode_fprintf(FILE *f, const struct patnode* node);


void patree_init(struct patree* tree);

/* Insert prefix to tree.
    Return the new node contain the prefix or old node contain the prefix.
    The prefix is almost same, e.g. will find 10.0.0.0/9 for 10.1.0.0/9.
    同 1 个 IP 不会学习两遍。

Lookup string the format of IP/MASK.
node contains prefix
*/
struct patnode* patree_lookup(struct patree* tree, struct patnode* node);

// Format IP/Mask to struct patnode
// return node  for success,
// return NULL for fail
struct patnode* patnode_format(struct patnode* node, const char* str);

struct patnode* patree_search_exact(const struct patree* tree, const char* p);

struct patnode* patree_search_best(const struct patree* tree, const char* p);

/* 左旋转 90 度打印树. */
void patree_fprintf(FILE * f, const struct patree* tree);

void patree_table(FILE * f, const struct patree* tree);

#endif /* _PATRICIA_H */
