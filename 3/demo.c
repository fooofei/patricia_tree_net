/*
 * $Id: demo.c,v 1.4 2005/12/07 20:55:52 dplonka Exp $
 *
 * This is based on "demo.c" provided with MRT-2.2.2a.
 https://github.com/tobez/Net-Patricia/blob/master/libpatricia/patricia.c

 */


#include <stdio.h> /* printf */
#include <stdlib.h> /* exit */
#include <string.h>

#include "crt_dbg_leak.h"

#include "patricia.h"

void test()
{
    struct patricia_tree mroot;
    memset(&mroot, 0, sizeof(mroot));
    struct patricia_tree * root = &mroot;

    patricia_init(root);

    patricia_lookup3(root, "127.0.0.0/8");


    patricia_lookup3(root, "10.42.42.0/24");
    patricia_lookup3(root, "10.42.69.0/24");
    patricia_lookup3(root, "10.0.0.0/8");
    patricia_lookup3(root, "10.0.0.0/9");


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
