
/*
  Only support IPv4
*/

#ifndef _PATRICIA_H
#define _PATRICIA_H

#include <stdint.h>
#include <stdio.h>

   
#define  INET_IPv4_ADDR_STR_SIZE 16
/* sizeof(struct in6_addr)*/
#define PATRICIA_MAXBITS	(sizeof(uint8_t)*16 * 8) 

struct prefix
{
    char string[INET_IPv4_ADDR_STR_SIZE + 1 + 2];
    char sin_str[INET_IPv4_ADDR_STR_SIZE];
    uint8_t mask;
    uint32_t sin;   // network order
};

/* Convert 127.0.0.1/32 to network order prefix struct. 
    If only give 127.0.0.1, then the mask default is 32.
  Return 0 on success.
*/
int prefix_ascii2prefix(struct prefix * self, const char * begin, const char * end);
/* 依据掩码做格式化，比如  10.1.0.0/9，被格式化为  10.0.0.0/9. */
void prefix_format(struct prefix * self);
void prefix_fprintf(const struct prefix * self, FILE * f);
uint8_t * prefix_to_networkorder_bytes(const struct prefix * self);

struct patricia_node
{
    struct patricia_node * left;
    struct patricia_node * right;
    struct patricia_node * parent;
    uint8_t mask;  /* 不一定等于 prefix->mask  可能是其他 */
    struct prefix * prefix;		/* who we are in patricia tree  TODO以后省内存 放指针 复用 */
    uint32_t is_alloced;
    /* for memory */
    struct prefix _a;
};
void patricia_node_fprintf(const struct patricia_node * self, FILE * f);

struct patricia_node_pool
{
    uint32_t  active_count;
    uint32_t  free_offset;
    uint32_t  capacity;
    struct patricia_node * ptr;
};

/* 这颗树的最大深度是 32 所以树中的递归不担忧栈溢出 */
struct patricia_tree
{
    struct patricia_node * root;
    uint32_t maxbits;/* for IP, 32 bit addresses  难道这个是所有结点的最大 mask ？ */
    struct patricia_node_pool pool;
};

struct patricia_node *  patricia_node_calloc(struct patricia_tree *);
struct patricia_node *  patricia_node_calloc2(struct patricia_tree *, const struct prefix * );
void patricia_node_free(struct patricia_tree *, struct patricia_node *);


void patricia_init(struct patricia_tree * );
void patricia_clear(struct patricia_tree *);

/* Insert prefix to tree.
   Return the new node contain the prefix or old node contain the prefix.
   The prefix is almost same, e.g. will find 10.0.0.0/9 for 10.1.0.0/9.
*/
struct patricia_node * patricia_lookup1(struct patricia_tree *, const char * begin, const char * end);
struct patricia_node * patricia_lookup2(struct patricia_tree *, const char * p);
void patricia_remove(struct patricia_tree * self, struct patricia_node *);

struct patricia_node * patricia_search_exact1(const struct patricia_tree *, const char * begin, const char * end);
struct patricia_node * patricia_search_exact2(const struct patricia_tree *, const char * p);
struct patricia_node * patricia_search_best1(const struct patricia_tree * self, const char * begin, const char * end);
struct patricia_node * patricia_search_best2(const struct patricia_tree * self, const char * p);


struct patricia_tree_iterator
{
    struct patricia_node * stack[PATRICIA_MAXBITS+1];
    struct patricia_node ** sp;
    struct patricia_node * rn;
};

void patricia_tree_iterator_set(struct patricia_tree_iterator * self, const struct patricia_tree *);
struct patricia_node * patricia_tree_iterator_next(struct patricia_tree_iterator * self, struct patricia_node ** out);

/* 左旋转 90 度打印树. */
void tree_fprintf(const struct patricia_tree * self, FILE * f);


#endif /* _PATRICIA_H */
