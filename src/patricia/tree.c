
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
#define max(a, b) (((a) (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif


/* Find the first different mask of p1 and p2.
   The return value is starts from 1.
*/

static uint8_t diff_mask(struct patree *tree, const struct prefix *p1,
                         const struct prefix *p2)
{
    uint32_t h1 = p1->host;
    uint32_t h2 = p2->host;
    uint32_t n1 = htonl(h1);
    uint32_t n2 = htonl(h2);

    uint8_t *addr1 = (uint8_t *) &(n1);
    uint8_t *addr2 = (uint8_t *) &(n2);
    uint8_t chk_maskbit = min(p1->maskbit, p2->maskbit);
    uint8_t diff_bit = 0;
    uint8_t i = 0;
    uint8_t diff_byte;
    uint8_t j = 0;

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
        diff_bit = i * 8 + j + 1;
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
static uint32_t test_maskbit(const struct patree *tree, uint32_t host, uint8_t mask)
{
    uint32_t a = 0x01;
    uint32_t b = a << (tree->maxbits - mask);
    return host & b;
}

/* insert node to child's parent.
newnode will be the child->parent's left or right.
*/
static void patree_insert(struct patree *tree, struct patnode *newnode, struct patnode *child)
{
    struct patnode *parent;
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

static void patree_fprintf1(FILE *f, const struct patnode *node, int space, int *dep)
{
    if (!node) {
        return;
    }
    (*dep)++;
    const int count = 10;
    space += count;

    patree_fprintf1(f, node->right, space, dep);

    fprintf(f, "%s", "\n");
    int i;
    for (i = count; i < space; i += 1) {
        fprintf(f, " ");
    }
    patnode_fprintf(f, node);
    fprintf(f, "%s", "\n");
    patree_fprintf1(f, node->left, space, dep);
    (*dep)--;
}


//
// static end
//



struct patnode *patree_search_exact(const struct patree *tree, const char *p)
{
    struct prefix pfx;
    memset(&pfx, 0, sizeof(pfx));
    int rc;

    rc = prefix_format(&pfx, p);
    if (rc < 0) {
        return NULL;
    }

    struct patnode *next;

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

    if (prefix_cmp(&pfx, next->prefix, next->maskbit)) {
        return next;
    }

    return NULL;
}

struct patnode *patree_search_best(const struct patree *tree, const char *p)
{
    struct patnode *stack[PATRICIA_MAXBITS + 1] = {0};
    struct patnode **stack_pointer;
    struct patnode *next;
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
        if (prefix_cmp(&pfx, next->prefix, next->maskbit)) {
            return next;
        }
        if (stack_pointer == stack) {
            break;
        }
    }
    return NULL;
}

static struct patnode *find_closest(struct patree *tree, struct prefix *lkp_prefix)
{
    struct patnode *closest = NULL;
    uint32_t lkp_host;
    uint8_t lkp_maskbit;

    lkp_host = lkp_prefix->host;
    lkp_maskbit = lkp_prefix->maskbit;
    closest = tree->root;
    for (; (closest != NULL && (closest->maskbit < lkp_maskbit ||
                                closest->prefix == NULL));) {
        if (closest->maskbit < tree->maxbits &&
            test_maskbit(tree, lkp_host, closest->maskbit)) {
            if (closest->right == NULL) {
                break;
            }
            closest = closest->right;
        } else {
            if (closest->left == NULL) {
                break;
            }
            closest = closest->left;
        }
    }
    return closest;
}

/* ref
https://github.com/opendaylight/lispflowmapping/blob/c7a0bda035cba583b090a78614f7c4209f1229a8/mappingservice/inmemorydb/src/main/java/org/opendaylight/lispflowmapping/inmemorydb/radixtrie/RadixTrie.java
*/

struct patnode *patree_lookup(struct patree *tree, struct patnode *lkp_node)
{
    if (tree->root == NULL) {
        tree->root = lkp_node;
        tree->node_cnt++;
        return tree->root;
    }

    struct patnode *parent = NULL;
    struct prefix *lkp_prefix = NULL;
    uint8_t lkp_maskbit = 0;
    uint32_t lkp_host = 0;
    struct patnode *closest = NULL;
    struct patnode *fstnode = NULL;

    lkp_prefix = lkp_node->prefix;
    lkp_maskbit = lkp_prefix->maskbit;
    lkp_host = lkp_prefix->host;

    closest = find_closest(tree, lkp_prefix);

    // 这个也不成立，一个 glue 结点，一定有左右孩子，
    // 下沉一定会到 glue 的孩子上，不会停留在 glue ，
    // 如果到了左右孩子，一定有 prefix
    //if (closest->prefix == NULL) {
    //    return NULL;
    //}

    // find first different bit<= min()
    uint8_t diff_bit = diff_mask(tree, closest->prefix, lkp_prefix);
    if (diff_bit >= tree->maxbits) {
        return NULL;
    }

    // find the first node with bit less than diffbit
    fstnode = closest;
    parent = fstnode->parent;
    for (; (parent != NULL && parent->maskbit >= diff_bit);) {
        fstnode = parent;
        parent = fstnode->parent;
    }

    if (diff_bit == fstnode->maskbit &&
        fstnode->maskbit == lkp_maskbit) {
        /* lookup 学习的 IP/MASK 是已经学习过的， 就会走到这里.
         如学习过 10.0.0.0/9, 再次 lookup  10.0.0.0/9 就会到这里，
         如学习过 10.0.0.0/9，再次 lookup  10.1.0.0/9(错误的值) 也会走到这里
       */

        // fstnode->prefix ? 如果这里代表是已经学习过的，
        // 那么 prefix 一定存在
        return fstnode;
    }
    if (diff_bit == fstnode->maskbit) {
        lkp_node->parent = fstnode;
        if (fstnode->maskbit < tree->maxbits &&
            test_maskbit(tree, lkp_host, fstnode->maskbit)) {
            if (fstnode->right != NULL) {
                return NULL;
            }
            fstnode->right = lkp_node;
        } else {
            if (fstnode->left != NULL) {
                return NULL;
            }
            fstnode->left = lkp_node;
        }
        tree->node_cnt++;
        return lkp_node;
    }

    if (diff_bit == lkp_maskbit) {
        uint32_t host;
        if (fstnode->prefix) {
            host = fstnode->prefix->host;
        } else {
            host = closest->prefix->host;
        }
        if (lkp_maskbit < tree->maxbits &&
            test_maskbit(tree, host, lkp_maskbit)) {
            lkp_node->right = fstnode;
        } else {
            lkp_node->left = fstnode;
        }
        patree_insert(tree, lkp_node, fstnode);
        tree->node_cnt++;
        return lkp_node;
    }
    // TODO memleak glue
    struct patnode *glue = calloc(1, sizeof(struct patnode));
    if (glue == NULL) {
        return NULL;
    }
    glue->maskbit = diff_bit;

    if (diff_bit < tree->maxbits &&
        test_maskbit(tree, lkp_host, diff_bit)) {
        glue->left = fstnode;
        glue->right = lkp_node;
    } else {
        glue->left = lkp_node;
        glue->right = fstnode;
    }
    patree_insert(tree, glue, fstnode);
    lkp_node->parent = glue;
    tree->node_cnt++;

    tree->glue_cnt++;
    glue->next = tree->glue_list;
    tree->glue_list = glue;

    return lkp_node;
}

void patree_fprintf(FILE *f, const struct patree *tree)
{
    fprintf(f, "-------------------------------------------\n");
    fprintf(f, "node=%d glue=%d\n", tree->node_cnt, tree->glue_cnt);
    int dep = 0;
    patree_fprintf1(f, tree->root, 0, &dep);
}

void patnode_fprintf(FILE *f, const struct patnode *node)
{
    fprintf(f, "%u-", node->maskbit);
    if (node->prefix) {
        prefix_fprintf(f, node->prefix);
    } else {
        fprintf(f, "<glue>");
    }
}

void patree_table(FILE *f, const struct patree *tree)
{
    fprintf(f, "-------------------------------------------\n");
    fprintf(f, "node=%d glue=%d\n", tree->node_cnt, tree->glue_cnt);
    int dep = 0;
    struct patnode * stack[PATRICIA_MAXBITS+1]={0};
    struct patnode ** stack_pointer;
    struct patnode * node;

    node = tree->root;
    stack_pointer = stack;
    for(;node;){
        struct patnode* left;
        struct patnode* right;

        patnode_fprintf(f, node);

        left = node->left;
        right = node->right;

        if (left) {
            if (right) {
                (*stack_pointer) = right;
                stack_pointer++;
            }
            node = left;
            fprintf(f, "  ");
        } else if (right) {
            node = right;
            fprintf(f, "\n");
        } else if (stack_pointer != stack) {
            stack_pointer--;
            node = *stack_pointer;
            fprintf(f, "\n");
        } else {
            node = NULL;
        }
    }
    fprintf(f,"\n");
}

void patree_init(struct patree *tree)
{
    memset(tree, 0, sizeof(*tree));
    tree->maxbits = 32;
    tree->node_cnt = 0;
    tree->glue_cnt = 0;
}

struct patnode *patnode_format(struct patnode *node, const char *str)
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

void patree_term(struct patree *tree)
{
    struct patnode **ppnode = NULL;
    struct patnode *del;

    for (ppnode = &tree->glue_list; *ppnode;) {
        del = *ppnode;
        (*ppnode) = del->next;
        free(del);
        tree->glue_cnt--;
    }
    memset(tree, 0, sizeof(*tree));
}
