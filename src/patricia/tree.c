
#include "tree.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#else

#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* insert node to child's parent.
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

static void patree_fprintf1(FILE* f, const struct patnode* node, int space, int* dep)
{
    (*dep)++;
    const int count = 10;
    space += count;

    if (node) {
        patree_fprintf1(f, node->right, space, dep);
    }
    fprintf(f, "%s", "\n");
    int i;
    for (i = count; i < space; i += 1) {
        fprintf(f, " ");
    }
    if (node) {
        patnode_fprintf(f, node);
    } else {
        fprintf(f, "<null>");
    }

    fprintf(f, "%s", "\n");
    if (node) {
        patree_fprintf1(f, node->left, space, dep);
    }
    (*dep)--;
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
    if (pfx.addr_family != tree->addr_family) {
        return NULL;
    }
    struct patnode* next;

    next = tree->root;
    const uint8_t maskbit = pfx.maskbit;

    for (; (next != NULL && next->maskbit < maskbit);) {
        if (prefix_test_bit(&pfx, next->maskbit + 1)) {
            next = next->right;
        } else {
            next = next->left;
        }
    }

    if (next == NULL || next->prefix == NULL || next->maskbit != maskbit) {
        return NULL;
    }

    if (prefix_cmp(&pfx, next->prefix, next->maskbit)) {
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
    if (pfx.addr_family != tree->addr_family) {
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
        if (prefix_test_bit(&pfx, next->maskbit + 1)) {
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

// 这个函数名字非常不容易理解
// 多个示例代码里都是这样的命名
// 其实真实含义是搜索某条匹配链表（把在二叉树走的路径视为一个双向链表）里最尾部的结点
//  
static struct patnode* find_closest(struct patree* tree, struct prefix* lkp_prefix)
{
    struct patnode* closest = NULL;
    uint8_t lkp_maskbit;

    lkp_maskbit = lkp_prefix->maskbit;
    closest = tree->root;
    for (; (closest != NULL && (closest->maskbit < lkp_maskbit || closest->prefix == NULL));) {
        if (closest->maskbit < tree->max_maskbit && prefix_test_bit(lkp_prefix, closest->maskbit + 1)) {
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

struct patnode* patree_lookup(struct patree* tree, struct patnode* lkp_node)
{
    if (lkp_node->prefix == NULL) {
        return NULL;
    }
    if (lkp_node->prefix->addr_family != tree->addr_family) {
        return NULL;
    }
    if (tree->root == NULL) {
        tree->root = lkp_node;
        tree->node_cnt++;
        return tree->root;
    }

    struct patnode* parent = NULL;
    struct prefix* lkp_prefix = NULL;
    uint8_t lkp_maskbit = 0;
    struct patnode* closest = NULL;
    struct patnode* fstnode = NULL;

    lkp_prefix = lkp_node->prefix;
    lkp_maskbit = lkp_prefix->maskbit;

    // 先在二叉树中寻找匹配最好的一个路径，也就是双向链表，返回最尾部的结点
    closest = find_closest(tree, lkp_prefix);

    // 这个也不成立，一个 glue 结点，一定有左右孩子，
    // 下沉一定会到 glue 的孩子上，不会停留在 glue ，
    // 如果到了左右孩子，一定有 prefix
    if (closest->prefix == NULL) {
        return NULL;
    }

    // find first different bit<= min()
    uint8_t final_same_bit = prefix_fst_diff_mask(closest->prefix, lkp_prefix) - 1;
    if (final_same_bit > tree->max_maskbit) {
        final_same_bit = tree->max_maskbit;
    }

    // find the first node with bit less than diffbit
    // 拿着最后一个相同的子网掩码位，从尾结点向前寻找最开始不匹配的，就是需要分叉的结点
    fstnode = closest;
    parent = fstnode->parent;
    for (; (parent != NULL && parent->maskbit > final_same_bit);) {
        fstnode = parent;
        parent = fstnode->parent;
    }

    // 最后一个相同的子网掩码位是自己，而且还是 fstnode（分叉结点），说明这个 fstnode 是自己
    if (final_same_bit == fstnode->maskbit && fstnode->maskbit == lkp_maskbit) {
        /* lookup 学习的 IP/MASK 是已经学习过的， 就会走到这里.
         如学习过 10.0.0.0/9, 再次 lookup  10.0.0.0/9 就会到这里，
       */
        // fstnode->prefix ? 如果这里代表是已经学习过的，
        // 那么 prefix 一定存在
        return fstnode;
    }
    
    if (final_same_bit == fstnode->maskbit) {
        lkp_node->parent = fstnode;
        if (fstnode->maskbit < tree->max_maskbit && prefix_test_bit(lkp_prefix, fstnode->maskbit + 1)) {
            if (fstnode->right != NULL) {
                ASSERT(fstnode->right == NULL);
                return NULL;
            }
            fstnode->right = lkp_node;
        } else {
            if (fstnode->left != NULL) {
                ASSERT(fstnode->left == NULL);
                return NULL;
            }
            fstnode->left = lkp_node;
        }
        tree->node_cnt++;
        return lkp_node;
    }

    if (final_same_bit == lkp_maskbit) {
        struct prefix* p;
        if (fstnode->prefix) {
            p = fstnode->prefix;
        } else {
            p = closest->prefix;
        }
        if (lkp_maskbit < tree->max_maskbit && prefix_test_bit(p, lkp_maskbit + 1)) {
            lkp_node->right = fstnode;
        } else {
            lkp_node->left = fstnode;
        }
        patree_insert(tree, lkp_node, fstnode);
        tree->node_cnt++;
        return lkp_node;
    }
    struct patnode* glue = calloc(1, sizeof(struct patnode));
    if (glue == NULL) {
        return NULL;
    }
    glue->maskbit = final_same_bit;

    if (final_same_bit < tree->max_maskbit && prefix_test_bit(lkp_prefix, final_same_bit + 1)) {
        glue->left = fstnode;
        glue->right = lkp_node;
    } else {
        glue->left = lkp_node;
        glue->right = fstnode;
    }
    patree_insert(tree, glue, fstnode);
    lkp_node->parent = glue;
    tree->node_cnt++;

    // add to glue list
    tree->glue_cnt++;
    glue->next = tree->glue_list;
    tree->glue_list = glue;

    return lkp_node;
}

void patree_fprintf(FILE* f, const struct patree* tree)
{
    fprintf(f, "-------------------------------------------\n");
    fprintf(f, "node=%d glue=%d\n", tree->node_cnt, tree->glue_cnt);
    int dep = 0;
    patree_fprintf1(f, tree->root, 0, &dep);
}

void patnode_fprintf(FILE* f, const struct patnode* node)
{
    fprintf(f, "%u-", node->maskbit);
    if (node->prefix) {
        prefix_fprintf(f, node->prefix);
    } else {
        fprintf(f, "<glue>");
    }
}

void patree_table(FILE* f, const struct patree* tree)
{
    fprintf(f, "-------------------------------------------\n");
    fprintf(f, "node=%d glue=%d\n", tree->node_cnt, tree->glue_cnt);
    int dep = 0;
    struct patnode* stack[PATRICIA_MAXBITS + 1] = { 0 };
    struct patnode** stack_pointer;
    struct patnode* node;

    node = tree->root;
    stack_pointer = stack;
    for (; node;) {
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
    fprintf(f, "\n");
}

int patree_init(struct patree* tree, uint8_t addr_family)
{
    memset(tree, 0, sizeof(*tree));
    tree->node_cnt = 0;
    tree->glue_cnt = 0;
    if (addr_family == AF_INET) {
        tree->addr_family = AF_INET;
        tree->max_maskbit = sizeof(struct in_addr) * 8;
    } else if (addr_family == AF_INET6) {
        tree->addr_family = AF_INET6;
        tree->max_maskbit = sizeof(struct in6_addr) * 8;
    } else {
        return -1;
    }
    return 0;
}

struct patnode* patnode_format(struct patnode* node, const char* str)
{
    int rc;
    memset(node, 0, sizeof(*node));
    node->prefix = &node->_a;
    node->prefix_string = node->prefix->string;
    rc = prefix_format(node->prefix, str);
    if (rc < 0) {
        memset(node, 0, sizeof(*node));
        return NULL;
    }
    node->maskbit = node->prefix->maskbit;
    return node;
}

void patree_term(struct patree* tree)
{
    struct patnode* next = NULL;
    struct patnode* del;

    next = tree->glue_list;
    for (; next;) {
        del = next;
        next = del->next;
        free(del);
        tree->glue_cnt--;
    }
    patree_init(tree, tree->addr_family);
}
