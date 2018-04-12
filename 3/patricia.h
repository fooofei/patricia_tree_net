
#ifndef _PATRICIA_H
#define _PATRICIA_H

#include <stdint.h>

struct prefix
{
    uint16_t bitlen;
    uint32_t sin;
};

struct patricia_node
{
    struct patricia_node * left;
    struct patricia_node * right;
    struct patricia_node * parent;
    uint32_t bit;	/* flag if this node used */
   struct prefix * prefix;		/* who we are in patricia tree */
   void *data;			/* pointer to data TODO */
   void	*user1;			/* pointer to usr data (ex. route flap info) TODO */
};

struct patricia_tree
{
    struct patricia_node * root;
    uint32_t maxbits;/* for IP, 32 bit addresses */
    uint32_t num_active_node;		/* for debug purpose */
};

struct patricia_tree * patricia_init(struct patricia_tree * );
struct patricia_tree * patricia_clear(struct patricia_tree *);

struct patricia_node * patricia_lookup(struct patricia_tree *, struct prefix *);


patricia_node_t *patricia_search_exact (patricia_tree_t *patricia, prefix_t *prefix);
patricia_node_t *patricia_search_best (patricia_tree_t *patricia, prefix_t *prefix);
patricia_node_t * patricia_search_best2 (patricia_tree_t *patricia, prefix_t *prefix, 
				   int inclusive);
patricia_node_t *patricia_lookup (patricia_tree_t *patricia, prefix_t *prefix);
void patricia_remove (patricia_tree_t *patricia, patricia_node_t *node);
patricia_tree_t *New_Patricia (int maxbits);
void Clear_Patricia (patricia_tree_t *patricia, void_fn_t func);
void Destroy_Patricia (patricia_tree_t *patricia, void_fn_t func);

void patricia_process (patricia_tree_t *patricia, void_fn_t func);

char *prefix_toa (prefix_t * prefix);

/* { from demo.c */

prefix_t *
ascii2prefix (int family, char *string);

patricia_node_t *
make_and_lookup (patricia_tree_t *tree, char *string);

patricia_node_t *
try_search_exact(patricia_tree_t *tree, char *string);

patricia_node_t *
try_search_best(patricia_tree_t *tree, char *string);

size_t
patricia_walk_inorder(patricia_node_t *node, void_fn_t func);
void
lookup_then_remove(patricia_tree_t *tree, char *string);

/* } */

#define PATRICIA_MAXBITS	(sizeof(struct in6_addr) * 8)
#define PATRICIA_NBIT(x)        (0x80 >> ((x) & 0x7f))
#define PATRICIA_NBYTE(x)       ((x) >> 3)

#define PATRICIA_DATA_GET(node, type) (type *)((node)->data)
#define PATRICIA_DATA_SET(node, value) ((node)->data = (void *)(value))

#define PATRICIA_WALK(Xhead, Xnode) \
    do { \
        patricia_node_t *Xstack[PATRICIA_MAXBITS+1]; \
        patricia_node_t **Xsp = Xstack; \
        patricia_node_t *Xrn = (Xhead); \
        while ((Xnode = Xrn)) { \
            if (Xnode->prefix)

#define PATRICIA_WALK_ALL(Xhead, Xnode) \
do { \
        patricia_node_t *Xstack[PATRICIA_MAXBITS+1]; \
        patricia_node_t **Xsp = Xstack; \
        patricia_node_t *Xrn = (Xhead); \
        while ((Xnode = Xrn)) { \
	    if (1)

#define PATRICIA_WALK_BREAK { \
	    if (Xsp != Xstack) { \
		Xrn = *(--Xsp); \
	     } else { \
		Xrn = (patricia_node_t *) 0; \
	    } \
	    continue; }

#define PATRICIA_WALK_END \
            if (Xrn->l) { \
                if (Xrn->r) { \
                    *Xsp++ = Xrn->r; \
                } \
                Xrn = Xrn->l; \
            } else if (Xrn->r) { \
                Xrn = Xrn->r; \
            } else if (Xsp != Xstack) { \
                Xrn = *(--Xsp); \
            } else { \
                Xrn = (patricia_node_t *) 0; \
            } \
        } \
    } while (0)

#endif /* _PATRICIA_H */
