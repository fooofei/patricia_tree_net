
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
    char sin_str[INET_IPv4_ADDR_STR_SIZE];
    uint32_t sin;   // network order
    uint8_t mask;
};

/* Convert 127.0.0.1/32 to network order prefix struct. 
    If only give 127.0.0.1, then the mask default is 32.
  Return 0 on success.
*/
int prefix_ascii2prefix(struct prefix * self, const char * begin, const char * end);
void prefix_fprintf(struct prefix * self, FILE * f);
uint8_t * prefix_to_networkorder_bytes(struct prefix * self);

struct patricia_node
{
    struct patricia_node * left;
    struct patricia_node * right;
    struct patricia_node * parent;
    uint32_t mask;  /* 不一定等于 prefix->mask  可能是其他 */
    struct prefix * prefix;		/* who we are in patricia tree  TODO以后省内存 放指针 复用 */
    //void *data;			/* pointer to data TODO */
    // void	*user1;			/* pointer to usr data (ex. route flap info) TODO */
    uint32_t is_alloced;
    /* for memory */
    struct prefix _a;
};

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
void patricia_node_free(struct patricia_tree *, struct patricia_node *);


void patricia_init(struct patricia_tree * );
void patricia_clear(struct patricia_tree *);

/* Insert prefix to tree.
    TODO Return what ?
    1 return the new node contain the prefix
*/
struct patricia_node * patricia_lookup(struct patricia_tree *, const char * begin, const char * end);
struct patricia_node * patricia_lookup3(struct patricia_tree *, const char * p);

struct patricia_node * patricia_search_exact(struct patricia_tree *, struct prefix *);
void patricia_remove(struct patricia_tree * self, struct patricia_node *);


struct patricia_tree_iterator
{
    struct patricia_node * stack[PATRICIA_MAXBITS];
    struct patricia_node ** sp;
    struct patricia_node * rn;
};

void patricia_tree_iterator_set(struct patricia_tree_iterator * self, const struct patricia_tree *);
struct patricia_node * patricia_tree_iterator_next(struct patricia_tree_iterator * self, struct patricia_node ** out);



#endif /* _PATRICIA_H */
