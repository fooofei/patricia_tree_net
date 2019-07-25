
#include "tree.h"

#include <assert.h>
#include <ctype.h> 
#include <errno.h> 
#include <math.h> 
#include <stddef.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef WIN32
#pragma comment(lib,"Ws2_32.lib")
#else

#endif

#ifndef max
#define max(a,b) (((a) (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif




/* Find the first different mask of curnode->prefix and prefix.
   The return value is starts from 0.
*/

static uint8_t diff_mask2(struct patree* tree, const struct prefix* cur, const struct prefix* pre)
{
    uint32_t host1 = cur->host;
    uint32_t host2 = pre->host;

    uint8_t chk_maskbit = min(cur->maskbit, pre->maskbit);

    uint8_t i;
    uint8_t j;

    return 0;
}
static uint8_t diff_mask(struct patree* tree, const struct prefix* p1,
                         const struct prefix* p2)
{
    uint8_t* addr1 = (uint8_t*) & (p1->sin);
    uint8_t* addr2 = (uint8_t*) & (p2->sin);
    uint8_t chk_maskbit = min(p1->maskbit, p2->maskbit);
    uint8_t diff_bit = 0;
    uint8_t i;
    uint8_t diff_byte;
    uint8_t j;

    // diff mask 应该叫 diff mask start
    for (i = 0; i * 8 < chk_maskbit; i += 1) {
        // 从左到右找
        diff_byte = (addr1[i] ^ addr2[i]);
        if (0 == diff_byte) {
            diff_bit = (i + 1) * 8;
            continue;
        }

        // 这是在找 最先不同的那一bit
        for (j = 0; j < 8; j += 1) {
            if (diff_byte & (0x80u >> j)) {
                break;
            }
        }
        diff_bit = i * 8 + j;
        break;
    }

    diff_bit = min(diff_bit, chk_maskbit);
    return diff_bit;
}

// 以 127.0.1.2 为例子
// 如果 mask=31，在看倒数第1位是不是1，
// mask=30, 在看倒数第2位是不是1
// mask=29, 在看倒数第3位是不是1
// 这是在找挨着掩码位的位置是不是1
static uint32_t test_maskbit(const struct patree* tree, uint32_t host, uint8_t mask)
{
    uint32_t a = 0x01;
    uint32_t b = a << (tree->maxbits - 1 - mask);
    return host & b;
}

/* insert_node to child's parent.
newnode will be the child->parent's left or right.
*/
static void patree_insert(struct patree* tree, struct patnode* newnode, struct patnode* child)
{
    struct patnode* parent;
    parent = child->parent;
    newnode->parent = parent;

    if (parent == NULL) {
        tree->root = newnode;
    } else if (parent->right == child) {
        parent->right = newnode;
    } else {
        parent->left = newnode;
    }
    child->parent = newnode;
}

static void patree_fprintf1(const struct patnode* node, int space, FILE* f)
{
    if (!node) {
        return;
    }
    const int count = 10;
    space += count;

    patree_fprintf1(node->right, space, f);

    fprintf(f, "%s", "\n");
    int i;
    for (i = count; i < space; i += 1) {
        fprintf(f, " ");
    }
    patnode_fprintf(node, f);
    fprintf(f, "%s", "\n");
    patree_fprintf1(node->left, space, f);
}

//
// static end
//



struct patnode* patree_search_exact(const struct patree* tree, const char* p)
{
    struct prefix pfx;
    memset(&pfx, 0, sizeof(pfx));
    int rc;

    rc = prefix_format(&pfx, p);
    if (rc < 0) {
        return NULL;
    }

    struct patnode* next;

    next = tree->root;
    const uint8_t maskbit = pfx.maskbit;

    for (; (next != NULL && next->maskbit < maskbit);) {
        if (test_maskbit(tree, pfx.host, next->maskbit)) {
            next = next->right;
        } else {
            next = next->left;
        }
    }

    if (next == NULL || next->prefix == NULL ||
        next->maskbit != maskbit) {
        return NULL;
    }

    if(prefix_cmp(&pfx, next->prefix)){
        return next;
    }

    return NULL;
}

struct patnode* patree_search_best(const struct patree* tree, const char* p)
{
    struct patnode* stack[PATRICIA_MAXBITS + 1] = { 0 };
    struct patnode** stack_pointer;
    struct patnode* next;
    struct prefix pfx;
    memset(&pfx, 0, sizeof(pfx));
    int rc;

    rc = prefix_format(&pfx, p);
    if (rc < 0) {
        return NULL;
    }
    if (tree->root == NULL) {
        return NULL;
    }
    next = tree->root;
    uint8_t maskbit = pfx.maskbit;

    stack_pointer = stack;
    for (; (next != NULL && (next->maskbit < maskbit));) {
        if (next->prefix) {
            *stack_pointer = next;
            stack_pointer++;
        }
        if (test_maskbit(tree, pfx.host, next->maskbit)) {
            next = next->right;
        } else {
            next = next->left;
        }
    }

    if (next && next->prefix) {
        *stack_pointer = next;
        stack_pointer++;
    }
    if (stack_pointer == stack) {
        return NULL;
    }

    for (;;) {
        stack_pointer--;
        next = *stack_pointer;
        if (prefix_cmp(&pfx, next->prefix)){
            return next;
        }
        if (stack_pointer == stack) {
            break;
        }
    }
    return NULL;
}

struct patnode* patree_lookup(struct patree* tree, struct patnode* lkp_node)
{
    if (tree->root == NULL) {
        tree->root = lkp_node;
        tree->node_cnt++;
        return tree->root;
    }

    struct patnode* next;
    struct prefix * lkp_prefix;
    uint8_t lkp_maskbit;
    uint32_t lkp_host;

    lkp_prefix = lkp_node->prefix;
    lkp_maskbit = lkp_prefix->maskbit;
    lkp_host = lkp_prefix->host;

    next = tree->root;
    for (; (next != NULL && (next->maskbit < lkp_maskbit ||
        next->prefix == NULL)); ) {
        if (next->maskbit < tree->maxbits &&
            test_maskbit(tree, lkp_host, next->maskbit)) {
            if (next->right == NULL) {
                break;
            }
            next = next->right;
        } else {
            if (next->left == NULL) {
                break;
            }
            next = next->left;
        }
    }

    if (next == NULL || next->prefix == NULL) {
        return NULL;
    }

    uint8_t diff_bit = diff_mask(tree, next->prefix, lkp_prefix);
    if (diff_bit >= tree->maxbits) {
        return NULL;
    }
    struct patnode* parent;
    struct patnode * next_bak;
    parent = next->parent;
    next_bak = next;
    for (; (parent != NULL && parent->maskbit >= diff_bit);) {
        next = parent;
        parent = next->parent;
    }

    if (diff_bit == next->maskbit &&
        next->maskbit == lkp_node->maskbit) {
        /* lookup 学习的 IP/MASK 是已经学习过的， 就会走到这里.
         如学习过 10.0.0.0/9, 再次 lookup  10.0.0.0/9 就会到这里，
         如学习过 10.0.0.0/9，再次 lookup  10.1.0.0/9(错误的值) 也会走到这里
       */

       // DEBUG prefix
        return next;
    }
    if (diff_bit == next->maskbit) {
        lkp_node->parent = next;
        if (next->maskbit < tree->maxbits &&
            test_maskbit(tree, lkp_node->prefix->host, next->maskbit)) {
            next->right = lkp_node;
        } else {
            next->left = lkp_node;
        }
        tree->node_cnt++;
        return lkp_node;
    }

    if (diff_bit == lkp_node->maskbit) {
        if (lkp_node->maskbit < tree->maxbits &&
            test_maskbit(tree, next_bak->prefix->host, lkp_node->maskbit)) {
            lkp_node->right = next;
        } else {
            lkp_node->left = next;
        }
        patree_insert(tree, lkp_node, next);
        tree->node_cnt++;
    } else {
        // TODO memleak glue
        struct patnode* glue = calloc(1, sizeof(struct patnode));
        glue->maskbit = diff_bit;

        if (diff_bit < tree->maxbits &&
            test_maskbit(tree, lkp_node->prefix->host, diff_bit)) {
            glue->left = next;
            glue->right = lkp_node;
        } else {
            glue->left = lkp_node;
            glue->right = next;
        }
        patree_insert(tree, glue, next);
        lkp_node->parent = glue;
        tree->node_cnt++;
    }
    return lkp_node;
}

void patree_fprintf(const struct patree* tree, FILE* f)
{
    fprintf(f, "-------------------------------------------\ncount=%d\n", tree->node_cnt);
    patree_fprintf1(tree->root, 0, f);
}

void patnode_fprintf(const struct patnode* node, FILE* f)
{
    fprintf(f, "%u-", node->maskbit);
    if (node->prefix) {
        prefix_fprintf(node->prefix, f);
    } else {
        fprintf(f, "null");
    }
}

void patree_init(struct patree* tree)
{
    memset(tree, 0, sizeof(*tree));
    tree->maxbits = 32;
    tree->node_cnt = 0;
}

struct patnode* patnode_format(struct patnode* node, const char* str)
{
    int rc;
    memset(node, 0, sizeof(*node));
    node->prefix = &node->_a;
    node->prefix_string = node->prefix->string;
    rc = prefix_format(node->prefix, str);
    if (rc < 0) {
        return NULL;
    }
    node->maskbit = node->prefix->maskbit;
    return node;
}
