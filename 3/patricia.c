

#include <assert.h> /* assert */
#include <ctype.h> /* isdigit */
#include <errno.h> /* errno */
#include <math.h> /* sin */
#include <stddef.h> /* NULL */
#include <stdio.h> /* sprintf, fprintf, stderr */
#include <stdlib.h> /* free, atol, calloc */
#include <string.h> /* memcpy, strchr, strlen */
#include <stdbool.h>


#include "patricia.h"
#include "../ipaddress.h"

#define MAXLINE 1024
#define BIT_TEST(f, b)  ((f) & (b))



/* 这是干什么用的？ 为什么还有这个 BIT_TEST(addr[node->bit >> 3], 0x80 >> (node->bit & 0x07)) */
// #define PATRICIA_NBIT(x)        (0x80 >> ((x) & 0x7f))
//#define PATRICIA_NBYTE(x)       ((x) >> 3)

#define PATRICIA_DATA_GET(node, type) (type *)((node)->data)
#define PATRICIA_DATA_SET(node, value) ((node)->data = (void *)(value))
#define PATRICIA_DEBUG 1 


static inline uint32_t _patricia_nbyte(uint32_t x)
{
    return x >> 3;
}
static inline uint32_t _patricia_nbit(uint32_t x)
{
    return  (uint32_t)(0x80 >> ((x) & 0x7));
}
static struct patricia_node * patricia_lookup2(struct patricia_tree * self, struct prefix * prefix);


bool comp_with_mask(const uint32_t *addr, const uint32_t *dest, uint8_t mask)
{

    if ( /* mask/8 == 0 || */ memcmp(addr, dest, mask / 8) == 0) {
        int n = mask / 8;
        int m = ((-1) << (8 - (mask % 8)));

        if (mask % 8 == 0 || (((const uint8_t *)addr)[n] & m) == (((const uint8_t *)dest)[n] & m))
            return (true);
    }
    return (false);
}

int prefix_ascii2prefix(struct prefix * self, const char * begin, const char * end)
{
    if (!(begin && end && self && begin < end))
    {
        return -1;
    }

    const char * sep;
    struct prefix temp;
    memset(&temp, 0, sizeof(temp));
    const uint8_t default_mask = sizeof(uint32_t) * 8;

    for (sep = begin; sep < end && (*sep != '/'); sep += 1);

    if (!(begin < sep))
    {
        return 0;
    }
    snprintf(temp.sin_str, sizeof(temp.sin_str), "%.*s", (int)(sep - begin), begin);
    ipaddr_pton(temp.sin_str, &temp.sin);
    
    temp.mask = default_mask;
    if (sep<end && ++sep<end)
    {
        char s[10] = {0};
        snprintf(s, sizeof(s), "%.*s", (int)(end - sep), sep);
        temp.mask = (uint8_t)strtol(s, 0, 10);
    }

    temp.mask = min(temp.mask, default_mask);
    memcpy(self, &temp, sizeof(*self));
    return 0;
}

void prefix_fprintf(struct prefix * self, FILE * f)
{
    fprintf(f, "%s/%u", self->sin_str, self->mask);
}

uint8_t * prefix_to_networkorder_bytes(struct prefix * self)
{
    return (uint8_t *)&self->sin;
}

void patricia_init(struct patricia_tree * self)
{
    memset(self, 0, sizeof(*self));
    self->maxbits = 32;
    self->pool.ptr = calloc(3000, sizeof(struct patricia_node));
    self->pool.capacity = 3000;
}

void patricia_clear(struct patricia_tree * self)
{
    struct patricia_tree_iterator it;
    memset(&it, 0, sizeof(it));
    struct patricia_node * cur = 0;

    patricia_tree_iterator_set(&it, self);


    for (; patricia_tree_iterator_next(&it, &cur);)
    {
    }


    assert(self->pool.active_count == 0);

    if (self->pool.ptr)
    {
        free(self->pool.ptr); self->pool.ptr = 0;
    }

}


void patricia_tree_iterator_set(struct patricia_tree_iterator * self, const struct patricia_tree * tree)
{
    memset(self, 0, sizeof(*self));
    self->sp = self->stack;
    self->rn = tree->root;
}

struct patricia_node * patricia_tree_iterator_next(struct patricia_tree_iterator * self, struct patricia_node ** out)
{
    *out = 0;
    if (!self->rn)
    {
        return 0;
    }

    *out = self->rn;

    /* Then move self->rn to next avaliable node.*/

    struct patricia_node * left;
    struct patricia_node * right;

    left = self->rn->left;
    right = self->rn->right;

    if (left)
    {
        if (right)
        {
            /* Push right to stack. */
            *(self->sp)++ = right; 
        }
        self->rn = left;
    }
    else if (right)
    {
        self->rn = right;
    }
    else if (self->sp != self->stack)
    {
        self->rn = *(--(self->sp));
    }
    else
    {
        /* Not have avaliable node. */
        self->rn = NULL;
    }
    return *out;
}


struct patricia_node * patricia_search_exact(struct patricia_tree * self, struct prefix * prefix)
{

    assert(prefix->mask <= self->maxbits);

    struct patricia_node * node;

    node = self->root;
    const uint32_t prefix_mask = prefix->mask;

    //for (;node && node->prefix.mask < prefix_mask;)
    {

    }

    return 0;
}

struct patricia_node *  patricia_node_calloc(struct patricia_tree * self)
{
    struct patricia_node * r = NULL;

    if (self->pool.free_offset >= self->pool.capacity)
    {
        return NULL;
    }

    r = &self->pool.ptr[self->pool.free_offset++];

    self->pool.active_count += 1;
    memset(r, 0, sizeof(*r));
    r->is_alloced = 1;
    r->prefix = &r->_a;
    return r;

}

void patricia_node_free(struct patricia_tree * self, struct patricia_node * r)
{
    r->is_alloced = 0;
    self->pool.active_count -= 1;
}

struct patricia_node * patricia_lookup3(struct patricia_tree * self, const char * p)
{
    return patricia_lookup(self, p, p + strlen(p));
}

struct patricia_node * patricia_lookup(struct patricia_tree * self, const char * begin, const char * end)
{
    struct prefix t;
    memset(&t, 0, sizeof(t));
    prefix_ascii2prefix(&t, begin, end);
    return patricia_lookup2(self, &t);
}

/* Find the first different mask of curnode->prefix and prefix. */

static uint32_t patricia_diff_mask(struct patricia_tree * self, struct patricia_node * curnode, struct prefix * prefix)
{
    uint8_t * node_addr = prefix_to_networkorder_bytes(curnode->prefix);
    uint8_t * prefix_addr = prefix_to_networkorder_bytes(prefix);
    uint8_t check_mask = min(curnode->mask, prefix->mask);
    uint8_t diff_mask = 0;
    uint8_t i;
    uint8_t r;
    uint8_t j;

    /* diff mask 应该叫 diff mask start */
    /* 这是在干啥 */
    for (i = 0; i * 8 < check_mask; i += 1)
    {
        /* 从左到右找 */
        if (0 == (r = (prefix_addr[i] ^ node_addr[i])))
        {
            diff_mask = (i + 1) * 8;
            continue;
        }

        /* 这是在找 最先不同的那一bit*/
        for (j = 0; j < 8; j += 1)
        {
            if (BIT_TEST(r, (0x80 >> j)))
            {
                break;
            }
        }

        /* TODO  must found ?*/
        if (!(j < 8))
        {
            return self->maxbits;
        }
        diff_mask = i * 8 + j;
        break;
    }

    diff_mask = min(diff_mask, check_mask);
    return diff_mask;
}

static struct patricia_node * patricia_lookup2(struct patricia_tree * self, struct prefix * prefix)
{
    if (self->root ==0)
    {
        self->root = patricia_node_calloc(self);
        self->root->mask = prefix->mask;
        memcpy(self->root->prefix, prefix, sizeof(struct prefix));
        return self->root;
    }

    struct patricia_node * node;
    uint8_t prefix_mask = prefix->mask;
    uint8_t * prefix_addr = prefix_to_networkorder_bytes(prefix);

    node = self->root;

    for (;node && node->mask< prefix_mask;)
    {
        /* TODO 为什么有第一个判断？ */
        /* 这里能提前取 a b 吗 会不会崩溃 */
        uint32_t a;
        uint32_t b;
        a = prefix_addr[_patricia_nbyte(node->mask)];
        b = _patricia_nbit(node->mask); /* 这是在找 1 */
        /* mask=32 看（右边起）第 8 位是不是 1 ， mask=31 看 第1 位是不是 1， mask=30 看第2位是不是1*/
        if (node->mask < self->maxbits && BIT_TEST(a,b) )
        {
            if (node->right == NULL)
            {
                break;
            }
            node = node->right;
        }
        else
        {
            if (node->left == NULL)
            {
                break;
            }
            node = node->left;
        }

    }

    if (!(node&& node->prefix))
    {
        return 0;
    }

    uint32_t diff_mask = patricia_diff_mask(self, node, prefix);
   
    struct patricia_node * parent;

    parent = node->parent;

    for (;parent && parent->mask >= diff_mask;)
    {
        node = parent;
        parent = node->parent;
    }

    if (diff_mask == prefix_mask && node->mask == prefix_mask)
    {

        /* TODO 校验这时候 node->prefix 与 prefix 关系 */

        assert(0 == memcmp(prefix, &node->prefix, sizeof(struct prefix)));

        return node;
    }

    struct patricia_node * new_node;

    new_node = patricia_node_calloc(self);
    memcpy(&new_node->prefix, prefix, sizeof(struct prefix));

    if (node->mask == diff_mask)
    {
        new_node->parent = node;
        uint32_t a;
        uint32_t b;
        a = prefix_addr[_patricia_nbyte(node->mask)];
        b = _patricia_nbit(node->mask);
        if (node->mask < self->maxbits && BIT_TEST(a, b))
        {
            node->right = new_node;
        }
        else
        {
            node->left = new_node;
        }
        return new_node;
    }

    if (prefix_mask == diff_mask)
    {
        if (prefix_mask<self->maxbits && BIT_TEST(node_addr[prefix_mask>>3], _patricia_nbit(prefix_mask)))
        {
            new_node->right = node;
        }
        else
        {
            new_node->left = node;
        }
        new_node->parent = node->parent;
        if (node->parent == 0)
        {
            self->root = new_node;
        }
        else if (node->parent->right==node)
        {
            node->parent->right = new_node;
        }
        else
        {
            node->parent->left = new_node;
        }
        node->parent = new_node;
    }
    else
    {
        struct patricia_node * glue;
        glue = patricia_node_calloc(self);
        glue->prefix = 0;
        glue->mask = diff_mask;
        glue->parent = node->parent;

        if (diff_mask < self->maxbits && BIT_TEST(prefix_addr[diff_mask>>3], _patricia_nbit(diff_mask)))
        {
            glue->right = new_node;
            glue->left = node;
        }
        else
        {
            glue->right = node;
            glue->left = new_node;
        }
        new_node->parent = glue;
        if (node->parent == 0)
        {
            self->root = glue;
        }
        else if (node->parent->right == node)
        {
            node->parent->right= glue;
        }
        else
        {
            node->parent->left = glue;
        }
        node->parent = glue;

    }
    return new_node;
}

void patricia_remove(struct patricia_tree * self, struct patricia_node * rm)
{
    struct patricia_node * parent;
    struct patricia_node * child;

    if (rm->left && rm->right)
    {
        if (rm->prefix)
        {
            rm->prefix = 0;
        }
        return ;
    }

    if (rm->right ==0 &&  rm->left ==0)
    {
        parent = rm->parent;
        patricia_node_free(self,rm);

        if (parent == 0)
        {
            self->root = 0;
            return;
        }
        else if (parent->right == rm)
        {
            parent->right = 0;
            child = parent->left;
        }
        else if (parent->left == rm)
        {
            parent->left = 0;
            child = parent->right;
        }

        if (parent->prefix)
        {
            return;
        }

        if (parent->parent==0)
        {
            assert(self->root == parent);
            self->root = child;
        }
        else if (parent->parent->right == parent)
        {
            parent->parent->right = child;
        }
        else
        {
            assert(parent->parent->left == parent);
            parent->parent->left = child;
        }
        child->parent = parent->parent;
        patricia_node_free(self, parent);
        return;

    }

    child = rm->left ? rm->left : rm->right;

    parent = rm->parent;
    child->parent = parent;

    patricia_node_free(self, rm);

    if (parent == 0)
    {
        assert(self->root == rm);
        self->root = child;
    }
    else if (parent->right == rm)
    {
        parent->right = child;
    }
    else {
        parent->left = child;
    }

}
