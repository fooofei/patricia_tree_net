/*
 * $Id: demo.c,v 1.4 2005/12/07 20:55:52 dplonka Exp $
 *
 * This is based on "demo.c" provided with MRT-2.2.2a.
 https://github.com/tobez/Net-Patricia/blob/master/libpatricia/patricia.c

 */


#include <stdio.h> /* printf */
#include <stdlib.h> /* exit */
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif
#include "crt_dbg_leak.h"

#include "patricia.h"

#ifdef __cplusplus
}
#endif


#define EXPECT(expr) \
    do { \
    if(!(expr)) \
        { \
        fprintf(stderr, "unexpect %s  (%s:%d)\n",#expr, __FILE__, __LINE__); \
        fflush(stderr);\
        } \
    } while (0)


void test()
{
    struct patricia_tree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patricia_tree * root = &mroot;

    patricia_init(root);

    patricia_lookup2(root, "127.0.0.0/8");


    patricia_lookup2(root, "10.42.42.0/24");


    patricia_lookup2(root, "10.42.69.0/24");

    patricia_lookup2(root, "10.0.0.0/8");
    patricia_lookup2(root, "10.0.0.0/9");
    tree_fprintf(root->root,  stdout);

    patricia_lookup2(root, "10.0.0.0/9");

    patricia_lookup2(root, "11.1.1.1/32");
    tree_fprintf(root->root, stdout);

    patricia_clear(root);


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
    struct patricia_tree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patricia_tree * root = &mroot;
    struct patricia_node * find;
    int i;
    const char * cidrs[] = {
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
    patricia_lookup2(root, cidrs[0]);

    find = patricia_search_best2(root, "127.0.0.1");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[0]));

    find = patricia_search_best2(root, "10.0.0.1");
    EXPECT(0 == find);

    for (i=1;0!=cidrs[i];i+=1)
    {
        patricia_lookup2(root, cidrs[i]);
    }

    tree_fprintf(root->root, stdout);

    find = patricia_search_best2(root, "10.42.42.0/24");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[1]));

    find = patricia_search_best2(root, "10.10.10.10");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[4]));

    find = patricia_search_best2(root, "10.10.10.1");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[4]));

    find = patricia_search_exact2(root, "10.0.0.0");
    EXPECT(0 == find);;
    find = patricia_search_exact2(root, "10.0.0.0/8");
    EXPECT(0 == strcmp(find->prefix->string, cidrs[3]));


    //
    tree_fprintf(root->root, stdout);

    patricia_clear(root);
}


void test_remove()
{
    struct patricia_tree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patricia_tree * root = &mroot;
    struct patricia_node * find;
    int i;
    const char * cidrs[] = {
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
 

    for (i = 0; 0 != cidrs[i]; i += 1)
    {
        patricia_lookup2(root, cidrs[i]);
    }

    tree_fprintf(root->root, stdout);

    struct patricia_tree_iterator it;
    memset(&it, 0, sizeof(it));
    patricia_tree_iterator_set(&it, root);
    struct patricia_node * vec[1024] = {0};
    int32_t cnt = 0;

    for (;patricia_tree_iterator_next(&it,&find);)
    {
        vec[cnt++] = find;
    }
    

    for (;--cnt>=0;)
    {
        fprintf(stdout,"%s", "\n-----------------------------\nRemove: ");
        patricia_node_fprintf(vec[cnt], stdout);
        fprintf(stdout, "%s", "\n");
        patricia_remove(root, vec[cnt]);
        tree_fprintf(root->root, stdout);
    }

    //

    patricia_clear(root);
}


int
main(void)
{
    struct _crt_dbg_leak dbg_leak;
    memset(&dbg_leak, 0, sizeof(dbg_leak));
    crt_dbg_leak_lock(&dbg_leak);

    test_remove();

    crt_dbg_leak_unlock(&dbg_leak);
    return 0;
}
