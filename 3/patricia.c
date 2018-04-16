

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


/* 这是干什么用的？ 为什么还有这个 BIT_TEST(addr[node->bit >> 3], 0x80 >> (node->bit & 0x07)) 
  原来代码好混乱
*/
// #define PATRICIA_NBIT(x)        (0x80 >> ((x) & 0x7f))
//#define PATRICIA_NBYTE(x)       ((x) >> 3)

static struct patricia_node * patricia_lookup(struct patricia_tree * self, const struct prefix * prefix);
static uint8_t patricia_bit_test(const uint8_t * addr, uint8_t mask);
static struct patricia_node * patricia_search_exact(const struct patricia_tree * self, const struct prefix * prefix);

static bool comp_with_mask(const void *addr, const void *dest, uint8_t mask)
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

    snprintf(self->string, sizeof(self->string), "%.*s", (int)(end - begin), begin);

    prefix_format(self);

    return 0;
}

void prefix_fprintf(const struct prefix * self, FILE * f)
{
    fprintf(f, "%s/%u", self->sin_str, self->mask);
}

uint8_t * prefix_to_networkorder_bytes(const struct prefix * self)
{
    return (uint8_t *)&self->sin;
}

void prefix_format(struct prefix * self)
{
    int8_t mask = self->mask;
    uint8_t maxmask = sizeof(uint32_t) * 8;
    uint32_t v;
    char * p = 0;

    if (0 == mask)
    {
        return;
    }
    mask = min(maxmask, mask);
    mask = maxmask - mask;
    v = (0x1 << mask) - 1;
    v = ~v;
    ipaddr_hton(v,&v);
    self->sin = self->sin & v;

    ipaddr_ntop(self->sin, &p);
    snprintf(self->sin_str, sizeof(self->sin_str), "%s", p);
    snprintf(self->string, sizeof(self->string), "%s/%u", p, self->mask);
    free(p);
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
        patricia_node_free(self, cur);
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

/* Only iterator the node have prefix, not include glue node. */
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

    if ((*out)->prefix == 0)
    {
        return patricia_tree_iterator_next(self, out);
    }
    return *out;
}


static struct patricia_node * patricia_search_exact(const struct patricia_tree * self, const struct prefix * prefix)
{

    assert(prefix->mask <= self->maxbits);

    struct patricia_node * node;

    node = self->root;
    const uint32_t prefix_mask = prefix->mask;
    const uint8_t * prefix_addr = prefix_to_networkorder_bytes(prefix);

    for (;node && node->mask < prefix_mask;)
    {
        if (patricia_bit_test(prefix_addr,node->mask))
        {
            node = node->right;
        }
        else
        {
            node = node->left;
        }
    }

    if (!(node && node->prefix && node->mask == prefix_mask))
    {
        return 0;
    }

    assert(node->mask == prefix_mask);
    assert(node->mask == node->prefix->mask);

    if (comp_with_mask(prefix_to_networkorder_bytes(node->prefix),
        prefix_to_networkorder_bytes(prefix), prefix_mask
    ))
    {
        return node;
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

struct patricia_node *  patricia_node_calloc2(struct patricia_tree * self , const struct prefix * pre)
{
    struct patricia_node * n;
    n = patricia_node_calloc(self);
    if (n)
    {
        memcpy(n->prefix, pre, sizeof(struct prefix));
        n->mask = pre->mask;
    }
    return n;
}

void patricia_node_free(struct patricia_tree * self, struct patricia_node * r)
{
    r->is_alloced = 0;
    self->pool.active_count -= 1;
}


/* Find the first different mask of curnode->prefix and prefix. 
  The return value is starts from 0.
*/

static uint32_t patricia_diff_mask(struct patricia_tree * self, const struct patricia_node * curnode, const  struct prefix * prefix)
{ 
#define BIT_TEST(f, b)  ((f) & (b))

    uint8_t * node_addr = prefix_to_networkorder_bytes(curnode->prefix);
    uint8_t * prefix_addr = prefix_to_networkorder_bytes(prefix);
    uint8_t check_mask = min(curnode->mask, prefix->mask);
    uint8_t diff_mask = 0;
    uint8_t i;
    uint8_t r;
    uint8_t j;

    /* diff mask 应该叫 diff mask start */
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

static uint8_t patricia_bit_test(const uint8_t * addr, uint8_t mask)
{
    uint8_t a = addr[mask >> 3];
    /* 0x80 is one bit */
    /* mask=32 看（右边起）第 8 位是不是 1 ， mask=31 看 第1 位是不是 1， mask=30 看第2位是不是1*/
    uint8_t b = (0x80 >> ((mask) & 0x7)); 
    return a & b;
}

/* Insert insert_node to child's parent. */
static void patricia_insert(struct patricia_tree * self, struct patricia_node * insert_node, struct patricia_node * child)
{
    insert_node->parent = child->parent;
    if (child->parent == 0)
    {
        self->root = insert_node;
    }
    else if (child->parent->right == child)
    {
        child->parent->right = insert_node;
    }
    else
    {
        assert(child->parent->left == child);
        child->parent->left = insert_node;
    }
    child->parent = insert_node;
}

static struct patricia_node * patricia_lookup(struct patricia_tree * self, const struct prefix * prefix)
{
    if (self->root ==0)
    {
        self->root = patricia_node_calloc2(self,prefix);
        return self->root;
    }

    struct patricia_node * node;
    uint8_t prefix_mask = prefix->mask;
    uint8_t * prefix_addr = prefix_to_networkorder_bytes(prefix);

    node = self->root;

    for (;node && (node->mask< prefix_mask || node->prefix==NULL);)
    {      
        if (node->mask < self->maxbits 
            && patricia_bit_test(prefix_addr, node->mask))
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
    if (diff_mask >= self->maxbits)
    {
        return 0;
    }
    /* 暂存 node 后面 node 会改变 */
    uint8_t * node_addr = prefix_to_networkorder_bytes(node->prefix);

    struct patricia_node * parent;

    parent = node->parent;

    for (;parent && parent->mask >= diff_mask;)
    {
        node = parent;
        parent = node->parent;
    }

    if (diff_mask == prefix_mask && node->mask == prefix_mask)
    {
        /* lookup 学习的 IP/MASK 是已经学习过的， 就会走到这里. 
          如学习过 10.0.0.0/9, 再次 lookup  10.0.0.0/9 就会到这里，
          如学习过 10.0.0.0/9，再次 lookup  10.1.0.0/9(错误的值) 也会走到这里
        */
        if (node->prefix)
        {
            assert(true == comp_with_mask(
                prefix_to_networkorder_bytes(prefix),
                prefix_to_networkorder_bytes(node->prefix), prefix_mask
            ));
        }
        // Not always true.
        //assert(0 == memcmp(prefix, node->prefix, sizeof(struct prefix)));

        return node;
    }

    struct patricia_node * new_node;

    new_node = patricia_node_calloc2(self, prefix);

    if (node->mask == diff_mask)
    {
        new_node->parent = node;

        if (node->mask < self->maxbits 
            && patricia_bit_test(prefix_addr,node->mask))
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
        if (prefix_mask<self->maxbits && patricia_bit_test(node_addr,prefix_mask))
        {
            new_node->right = node;
        }
        else
        {
            new_node->left = node;
        }
        patricia_insert(self, new_node, node);
    }
    else
    {
        struct patricia_node * glue;
        glue = patricia_node_calloc(self);
        glue->prefix = 0;
        glue->mask = diff_mask;

        if (diff_mask < self->maxbits && patricia_bit_test(prefix_addr, diff_mask))
        {
            glue->right = new_node;
            glue->left = node;
        }
        else
        {
            glue->right = node;
            glue->left = new_node;
        }
        patricia_insert(self, glue, node);
        new_node->parent = glue;

    }
    return new_node;
}

static struct patricia_node * patricia_search_best(const struct patricia_tree * self, const struct prefix * prefix)
{
    struct patricia_node * stack[PATRICIA_MAXBITS+1];
    int32_t cnt = 0;
    struct patricia_node * node;

    assert(prefix);
    assert(prefix->mask <= self->maxbits);

    if (!self->root)
    {
        return 0;
    }

    node = self->root;
    const uint8_t * prefix_addr = prefix_to_networkorder_bytes(prefix);
    uint8_t prefix_mask = prefix->mask;

    cnt = 0;
    for (;node && (node->mask < prefix_mask);)
    {
        if (node->prefix)
        {
            stack[cnt++] = node;
        }

        if (patricia_bit_test(prefix_addr,node->mask))
        {
            node = node->right;
        }
        else
        {
            node = node->left;
        }
    }

    /* 这里可以控制查找是否包括自身，比如要查找 1.1.0.0/16 ，如果该值已经被 lookup(学习),这句话可以控制是否把
      查找树中的 1.1.0.0/16 也加入查找
    */
    if (node && node->prefix)
    {
        stack[cnt++] = node;
    }

    if (cnt <= 0)
    {
        return 0;
    }

    for (;--cnt>=0;)
    {
        node = stack[cnt];

        if (comp_with_mask(prefix_to_networkorder_bytes(prefix),prefix_to_networkorder_bytes(node->prefix),node->prefix->mask))
        {
            return node;
        }
    }
    return 0;
}

void patricia_remove(struct patricia_tree * self, struct patricia_node * rm)
{
    struct patricia_node * parent;
    struct patricia_node * child;

    if (rm->left && rm->right)
    {
        // Make as a glue.
        if (rm->prefix)
        {
            rm->prefix = 0;
        }
    }

    else if (rm->right ==0 &&  rm->left ==0)
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

    }
    else
    {
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
}

void patricia_node_fprintf(const struct patricia_node * self, FILE * f)
{
    fprintf(f, "%u-", self->mask);
    if (self->prefix)
    {
        prefix_fprintf(self->prefix, f);
    }
    else
    {
        fprintf(f, "null");
    }
}

static void tree_fprintf_1(const struct patricia_node * node, int space, FILE * f)
{
    if (!node)
    {
        return;
    }
    const int count = 10;
    space += count;

    tree_fprintf_1(node->right, space, f);

    fprintf(f,"%s", "\n");
    int i;
    for (i = count; i < space; i += 1) fprintf(f, " ");
    patricia_node_fprintf(node, f);
    fprintf(f, "%s", "\n");
    tree_fprintf_1(node->left, space, f);
}

void tree_fprintf(const struct patricia_node * node, FILE * f)
{
    fprintf(f,"%s", "-------------------------------------------\n");
    tree_fprintf_1(node, 0, f);
}


struct patricia_node * patricia_lookup2(struct patricia_tree * self, const char * p)
{
    return patricia_lookup1(self, p, p + strlen(p));
}

struct patricia_node * patricia_lookup1(struct patricia_tree * self, const char * begin, const char * end)
{
    struct prefix t;
    memset(&t, 0, sizeof(t));
    prefix_ascii2prefix(&t, begin, end);
    return patricia_lookup(self, &t);
}

struct patricia_node * patricia_search_exact1(const struct patricia_tree * self, const char * begin, const char * end)
{
    struct prefix t;
    memset(&t, 0, sizeof(t));
    prefix_ascii2prefix(&t, begin, end);
    return patricia_search_exact(self, &t);
}
struct patricia_node * patricia_search_exact2(const struct patricia_tree * self, const char * p)
{
    return patricia_search_exact1(self, p, p + strlen(p));
}

struct patricia_node * patricia_search_best1(const struct patricia_tree * self, const char * begin, const char * end)
{
    struct prefix t;
    memset(&t, 0, sizeof(t));
    prefix_ascii2prefix(&t, begin, end);
    return patricia_search_best(self, &t);
}

struct patricia_node * patricia_search_best2(const struct patricia_tree * self, const char * p)
{
    return patricia_search_best1(self, p, p + strlen(p));
}
