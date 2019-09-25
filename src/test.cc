/*
https://github.com/tobez/Net-Patricia/blob/master/libpatricia/patricia.c

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>


#include "crt_dbg_leak.h"

extern "C" {
#include "patricia/tree.h"
#include "patricia/iterator.h"
}


#define EXPECT(expr) \
    do { \
    if(!(expr)) \
        { \
        fprintf(stderr, "unexpect %s  (%s:%d)\n",#expr, __FILE__, __LINE__); \
        fflush(stderr);\
        } \
    } while (0)


typedef std::list<struct patnode *> list_node_t;

static void patree_format_m(list_node_t &nodes, struct patree *tree, const char *str)
{
    struct patnode *node;
    struct patnode *rc_node;
    node = (struct patnode *) calloc(1, sizeof(struct patnode));
    rc_node = patnode_format(node, str);
    if(rc_node == NULL){
        printf("fail parse %s\n", str);
    }
    if(rc_node){
        int cnt_bak = tree->node_cnt;
        rc_node = patree_lookup(tree, node);
        if(rc_node == NULL){
            printf("fail lookup %s\n", str);
            patree_lookup(tree, node);
        }
        if (rc_node && cnt_bak == tree->node_cnt){
            fprintf(stdout, "lookup: ");
            patnode_fprintf(stdout,node);
            fprintf(stdout, " same with: ");
            patnode_fprintf(stdout, rc_node);
            fprintf(stdout, "\n");
        }
    }

    nodes.push_back(node);
}

static void patree_node_term(list_node_t &nodes, struct patree *tree)
{
    patree_term(tree);
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
    struct patree *tree = &mroot;
    std::list<struct patnode *> nodes;

    patree_init(tree, AF_INET);

    const char *cidrs[] = {
            "127.0.0.0/8",
            "10.42.42.0/24",
            "10.42.69.0/24",
            "10.0.0.0/8",
            "10.0.0.0/9",
            NULL,
    };
    const char **p = NULL;
    for (p = cidrs; *p != NULL; p++) {
        patree_format_m(nodes, tree, *p);
    }
    patree_table(stdout, tree);

    patree_format_m(nodes, tree, "10.0.0.0/9");

    patree_format_m(nodes, tree, "11.1.1.1/32");
    patree_table(stdout, tree);

    patree_node_term(nodes, tree);

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
    struct patree *tree = &mroot;
    struct patnode *find;
    int i;
    list_node_t nodes;
    const char *cidrs[] = {
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


    patree_init(tree, AF_INET);
    //
    patree_format_m(nodes, tree, cidrs[0]);

    find = patree_search_best(tree, "127.0.0.1");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[0]));

    find = patree_search_best(tree, "10.0.0.1");
    EXPECT(0 == find);

    //FILE * f;
    //f = fopen("D:\\1.txt", "wb");

    for (i = 1; 0 != cidrs[i]; i += 1) {
        patree_format_m(nodes, tree, cidrs[i]);
    }
    //fclose(f);

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

    patree_table(stdout, tree);
    //
    patree_node_term(nodes, tree);
}


void test3()
{
    struct patree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patree *tree = &mroot;
    struct patnode *find;
    list_node_t nodes;
    int i;
    const char *cidrs[] = {
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


    patree_init(tree, AF_INET);

    for (i = 0; 0 != cidrs[i]; i += 1) {
        patree_format_m(nodes, tree, cidrs[i]);
    }

    patree_table(stdout, tree);

    struct patree_iterator it;
    memset(&it, 0, sizeof(it));
    patree_iter_set(&it, tree);
    struct patnode *vec[1024] = {0};
    int cnt = 0;

    for (; patree_iter_next(&it, &find);) {
        if (find->prefix) {
            vec[cnt++] = find;
        }
    }


    for (; --cnt >= 0;) {
        //fprintf(stdout, "%s", "\n-----------------------------\nRemove: ");
        patnode_fprintf(stdout, vec[cnt]);
        fprintf(stdout, "%s", "\n");
    }

    //
    patree_node_term(nodes, tree);
}

void test_glue()
{
    struct patree treem;
    memset(&treem, 0, sizeof(treem));
    struct patree *tree = &treem;
    list_node_t nodes;
    const char **pp;

    patree_init(tree, AF_INET);

    const char *cidrs[] = {
            "127.3.0.1/16",
            "127.42.42.0/24",
            "127.0.0.1/1",
            "127.127.127.127/32",
            NULL,
    };

    pp = cidrs;
    for (; (*pp); pp++) {
        struct patnode * node = NULL;
        node = patree_search_best(tree, *pp);
        patree_format_m(nodes, tree, *pp);
    }

    patree_node_term(nodes, tree);
}

void test_32bitmask()
{
    struct patree treem;
    memset(&treem, 0, sizeof(treem));
    struct patree *tree = &treem;
    list_node_t nodes;
    const char **pp;

    patree_init(tree, AF_INET);

    const char *cidrs[] = {
            "127.128.128.128/32",
            "228.227.229.226/32",
            "209.129.128.126/32",
            NULL,
    };

    pp = cidrs;
    for (; (*pp); pp++) {
        patree_format_m(nodes, tree, *pp);
    }

    patree_search_best(tree, "209.129.128.126");

    patree_node_term(nodes, tree);
}

void test_ipv6()
{
    struct patree treem;
    memset(&treem, 0, sizeof(treem));
    struct patree *tree = &treem;
    list_node_t nodes;
    const char **pp;

    patree_init(tree, AF_INET6);

    const char *cidrs[] = {
            "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/16",
            "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/32",
            NULL,
    };

    pp = cidrs;


    for (; (*pp); pp++) {
    }
    struct patnode * node = NULL;
    node = patree_search_best(tree, cidrs[0]);
    EXPECT(node == NULL);
    patree_format_m(nodes, tree, *pp);

    node = patree_search_best(tree, cidrs[1]);
    EXPECT(node != NULL);
    EXPECT(0 == strcmp(node->prefix_string,cidrs[0]));
    patree_format_m(nodes, tree, cidrs[1]);

    patree_node_term(nodes, tree);
}

// 20190925 我可总算是学会了

int main(void)
{
    struct _crt_dbg_leak dbg_leak;
    memset(&dbg_leak, 0, sizeof(dbg_leak));
    crt_dbg_leak_lock(&dbg_leak);

    test_32bitmask();
    test_glue();
    test();
    test1();
    test3();
    test_ipv6();
    crt_dbg_leak_unlock(&dbg_leak);
    return 0;
}
