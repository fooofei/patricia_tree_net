/*
https://github.com/tobez/Net-Patricia/blob/master/libpatricia/patricia.c

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>


#include "crt_dbg_leak.h"
#include "patricia/tree.h"


#define EXPECT(expr) \
    do { \
    if(!(expr)) \
        { \
        fprintf(stderr, "unexpect %s  (%s:%d)\n",#expr, __FILE__, __LINE__); \
        fflush(stderr);\
        } \
    } while (0)


typedef std::list<struct patnode*> list_node_t;

static void patree_format_m(list_node_t& nodes, struct patree* root, const char* str)
{
    struct patnode* node;
    struct patnode* rc_node;
    node = (struct patnode*)calloc(1, sizeof(struct patnode));
    rc_node = patnode_format(node, str);
    rc_node = patree_lookup(root, node);
    nodes.push_back(node);
}

static void patree_term(list_node_t& nodes)
{
    list_node_t::iterator it;
    for (it = nodes.begin(); it != nodes.end(); it++) {
        free(*it);
    }
    nodes.clear();
}

void test()
{
    struct patree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patree* tree = &mroot;
    std::list<struct patnode*> nodes;

    patree_init(tree);

    const char* cidrs[] = {
        "127.0.0.0/8",
        "10.42.42.0/24",
        "10.42.69.0/24",
        "10.0.0.0/8",
        "10.0.0.0/9",
        NULL,
    };
    const char** p = NULL;
    for (p = cidrs; *p != NULL; p++) {
        patree_format_m(nodes, tree, *p);
    }
    patree_fprintf(tree, stdout);

    patree_format_m(nodes, tree, "10.0.0.0/9");

    patree_format_m(nodes, tree, "11.1.1.1/32");
    patree_fprintf(tree, stdout);

    patree_term(nodes);

    // insert 127.0.0.0/8
    //       127.0.0.0 - 8 / 8


    // 127.0.0.0 / 8
    // 10.42.42.0 / 24
    //                0-0/1
    //             /          \
    //            /            \
    // 10.42.42.0 - 24 / 24    127.0.0.0 - 8 / 8



    // 127.0.0.0 / 8
    // 10.42.42.0 / 24
    // 10.42.69.0 / 24
    //                            0-0/1
    //                         /          \
    //                        /            \
    //                 0 - 0 / 17       127.0.0.0 - 8 / 8
    //                /          \
    // 10.42.42.0 - 24 / 24  10.42.69.0 - 24 / 24



    // 127.0.0.0 / 8
    // 10.42.42.0 / 24
    // 10.42.69.0 / 24
    // 10.0.0.0 / 8

    //                                        0-0/1
    //                                     /          \
    //                                    /            \
    //                          10.0.0.0 - 8 / 8     127.0.0.0 - 8 / 8
    //                               /
    //                          0 - 0 / 17
    //                         /      \
    //       10.42.42.0 - 24 / 24    10.42.69.0 - 24 / 24





    // 127.0.0.0 / 8
    // 10.42.42.0 / 24
    // 10.42.69.0 / 24
    // 10.0.0.0 / 8
    // 10.0.0.0 / 9

    //                         root(0 - 0 / 1)
    //                        /                \
    //                    10.0.0.0 - 8 / 8     127.0.0.0-8/8
    //                     /           
    //                 10.0.0.0-9/9
    //                  /       
    //                0-0/17
    //                 /    \
    //     10.42.42.0-24/24  10.42.69.0-24/24



}

void test1()
{
    struct patree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patree* tree = &mroot;
    struct patnode* find;
    int i;
    list_node_t nodes;
    const char* cidrs[] = {
        "127.0.0.0/8",
        "10.42.42.0/24",
        "10.42.69.0/24",
        "10.0.0.0/8",
         "10.0.0.0/9",
        "10.1.0.0/9",  // 与  "10.0.0.0/9", 等价 为了测试查找时 栈够不够用
        "10.2.0.0/9",
        "10.3.0.0/9",
        "10.4.0.0/9",
        "10.5.0.0/9",
        "10.6.0.0/9",
        "1.123.160.22",
        "1.123.160.29",
        "1.1.1.0",
        "1.123.160.3",
        "1.1.1.112",
        "1.1.1.1",
        "1.1.1.2",
        "1.123.160.21",
        "1.123.160.28",
        "1.123.160.2",
        "1.1.1.111",
        "2.2.2.2,3.3.3.3",
        "2.2.2.2,3.3.3.3",
        "4.2.5.5",
        "7.8.2.3",
        "12.5.63.0/24",
        "8.0.0.0/12",
        "8.8.8.8,70.1.1.0/24",
        "1.2.56.0/24",
        "6.6.5.0/24,4.4.5.5,4.6.4.0/23",
        "6.6.5.0/24,4.4.5.5,4.6.4.0/23",
        "0.0.0.0/1,3.3.3.2/31",
        "18.18.18.1",
        "1.123.160.22",
        "1.123.160.29",
        "1.123.160.2",
        "1.1.1.112",
        "1.123.160.21",
        "1.123.160.28",
        "1.123.160.3",
        "1.1.1.111",
        "2.2.2.2,3.3.3.3",
        "2.2.2.2,3.3.3.3",
        "4.2.5.5",
        "7.8.2.3",
        "12.5.63.0/24",
        "8.0.0.0/12",
        "8.8.8.8,70.1.1.0/24",
        "1.2.56.0/24",
        "6.6.5.0/24,4.4.5.5,4.6.4.0/23",
        "6.6.5.0/24,4.4.5.5,4.6.4.0/23",
        "0.0.0.0/1,3.3.3.2/31",
        "18.18.18.1",
        0
    };


    patree_init(tree);
    //
    patree_format_m(nodes, tree, cidrs[0]);

    find = patree_search_best(tree, "127.0.0.1");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[0]));

    find = patree_search_best(tree, "10.0.0.1");
    EXPECT(0 == find);

    //FILE * f;
    //f = fopen("D:\\1.txt", "wb");

    for (i = 1; 0 != cidrs[i]; i += 1) {
        patree_fprintf(tree, stdout); fflush(stdout);
        patree_format_m(nodes, tree, cidrs[i]);
    }
    //fclose(f);

    patree_fprintf(tree, stdout);

    find = patree_search_best(tree, "10.42.42.0/24");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[1]));

    find = patree_search_best(tree, "10.10.10.10");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[4]));

    find = patree_search_best(tree, "10.10.10.1");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[4]));

    find = patree_search_exact(tree, "10.0.0.0");
    EXPECT(0 == find);;
    find = patree_search_exact(tree, "10.0.0.0/8");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[3]));


    //
    patree_fprintf(tree, stdout); fflush(stdout);

    patree_term(nodes);
}

#if 0

void test_remove()
{
    struct patricia_tree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patricia_tree* root = &mroot;
    struct patricia_node* find;
    int i;
    const char* cidrs[] = {
        "127.0.0.0/8",
        "10.42.42.0/24",
        "10.42.69.0/24",
        "10.0.0.0/8",
        "10.0.0.0/9",
        "10.1.0.0/9",  // 与  "10.0.0.0/9", 等价 为了测试查找时 栈够不够用
        "10.2.0.0/9",
        "10.3.0.0/9",
        "10.4.0.0/9",
        "10.5.0.0/9",
        "10.6.0.0/9",
        0
    };


    patricia_init(root);
    //


    for (i = 0; 0 != cidrs[i]; i += 1) {
        patricia_lookup2(root, cidrs[i]);
    }

    tree_fprintf(root, stdout);

    struct patricia_tree_iterator it;
    memset(&it, 0, sizeof(it));
    patricia_tree_iterator_set(&it, root);
    struct patricia_node* vec[1024] = { 0 };
    int32_t cnt = 0;

    for (; patricia_tree_iterator_next(&it, &find);) {
        if (find->prefix) {
            vec[cnt++] = find;
        }
    }


    for (; --cnt >= 0;) {
        fprintf(stdout, "%s", "\n-----------------------------\nRemove: ");
        patricia_node_fprintf(vec[cnt], stdout);
        fprintf(stdout, "%s", "\n");
        patricia_remove(root, vec[cnt]);
        tree_fprintf(root, stdout);
    }

    //

    patricia_clear(root);
}

#endif


int
main(void)
{
    struct _crt_dbg_leak dbg_leak;
    memset(&dbg_leak, 0, sizeof(dbg_leak));
    crt_dbg_leak_lock(&dbg_leak);

    test();

    crt_dbg_leak_unlock(&dbg_leak);
    return 0;
}
