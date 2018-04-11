/*
 * patricia_test.c
 * Patricia Trie test code.
 *
 * Compiling the test code (or any other file using libpatricia):
 *
 *     gcc -g -Wall -I. -L. -o ptest patricia_test.c -lpatricia
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#ifdef WIN32
#include <WinSock2.h>
#else
 #include <unistd.h>

#include <sys/socket.h>
 #include <sys/time.h>
 #include <sys/types.h>
 #include <sys/wait.h>

 #include <rpc/rpc.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
#endif


#pragma warning(disable:4996)

#include "patricia.h"
#include "../ipaddress.h"

/*
 * Arbitrary data associated with a given node.
 */
struct Node {
  int    foo;
  double bar;
};


int test1(int argc, char **argv) {
  struct ptree *root;
  struct ptree *p, *pfind;
  struct ptree_mask *pm;
  FILE *fp;
  char line[128];
  //char addr_str[16];
  struct in_addr addr;
  uint32_t mask = 0xffffffff;
  float time;
  uint32_t host;

//   if (argc < 2) {
//     printf("Usage: %s <TCP stream>\n", argv[0]);
//     exit(-1);
//   }

  const char * fname = "../small.udp";
  /*
   * Open file of IP addresses and masks.
   * Each line looks like:
   *    10.0.3.4 0xffff0000
   */
  if ((fp = fopen(fname, "r")) == NULL) {
    printf("File %s doesn't seem to exist\n", fname);
    exit(1);
  }

  /*
   * Initialize the Patricia trie by doing the following:
   *   1. Assign the head pointer a default route/default node
   *   2. Give it an address of 0.0.0.0 and a mask of 0x00000000
   *      (matches everything)
   *   3. Set the bit position (p_b) to 0.
   *   4. Set the number of masks to 1 (the default one).
   *   5. Point the head's 'left' and 'right' pointers to itself.
   * NOTE: This should go into an intialization function.
   */
  root = (struct ptree *)calloc(1,sizeof(struct ptree));

  if (!root) {
    perror("Allocating p-trie node");
    exit(1);
  }

  root->p_m = (struct ptree_mask *)calloc(1,sizeof(struct ptree_mask));

  if (!root->p_m) {
    perror("Allocating p-trie mask data");
    exit(1);
  }

  pm = root->p_m;
  pm->pm_data = (struct Node *)calloc (1,sizeof(struct Node));

  if (!pm->pm_data) {
    perror("Allocating p-trie mask's node data");
    exit(1);
  }
  /*******
   *
   * Fill in default route/default node data here.
   *
   *******/
  root->p_mlen = 1;
  root->p_left = root->p_right = root;

  /*
   * The main loop to insert nodes.
   */
  while (fgets(line, 128, fp)) {
    /*
     * Read in each IP address and mask and convert them to
     * more usable formats.
     */
      time = 0;
      int p1, p2, p3, p4;
    sscanf(line, "%d.%d.%d.%d/%d",&p1,&p2,&p3,&p4,&mask
    );
    // inet_aton(addr_str, &addr);
    // host order
    addr.s_addr = (p1 << 24) | (p2 << 16) | (p3 << 8) | p4;
    addr.s_addr = htonl(addr.s_addr);

    /*
     * Create a Patricia trie node to insert.
     */
    p = (struct ptree *)calloc(1, sizeof(struct ptree));

    if (!p) {
      perror("Allocating p-trie node");
      exit(1);
    }


    /*
     * Allocate the mask data.
     */
    p->p_m = (struct ptree_mask *)calloc(1, sizeof(struct ptree_mask));

    if (!p->p_m) {
      perror("Allocating p-trie mask data");
      exit(1);
    }


    /*
     * Allocate the data for this node.
     * Replace 'struct Node' with whatever you'd like.
     */
    pm = p->p_m;
    pm->pm_data = (struct Node *)calloc(1,sizeof(struct Node));

    if (!pm->pm_data) {
      perror("Allocating p-trie mask's node data");
      exit(1);
    }

    /*
     * Assign a value to the IP address and mask field for this
     * node.
     */
    p->p_key = addr.s_addr; /* Network-byte order */
    p->p_m->pm_mask = htonl(mask);

    host = 0;
    ipaddr_pton("1.1.2.2", &host);
    pfind = pat_search(host, root);

    host = 0;
    ipaddr_pton("2.2.2.3", &host);
    pfind = pat_search(host, root);

    pfind = pat_search(addr.s_addr, root);

    // printf("%08x %08x %08x\n",p->p_key, addr.s_addr, p->p_m->pm_mask);
    // if (pfind->p_key==(addr.s_addr&pfind->p_m->pm_mask))
    if (pfind && pfind->p_key == addr.s_addr) {
      printf("%f %08x: ", time, addr.s_addr);
      printf("Found.\n");
    } else {

     /*
      * Insert the node.
      * Returns the node it inserted on success, 0 on failure.
      */
      // printf("%08x: ", addr.s_addr);
      // printf("Inserted.\n");
      p = pat_insert(p, root);
    }

    if (!p) {
      fprintf(stderr, "Failed on pat_insert\n");
      exit(1);
    }
  }

  exit(0);
}


void test2()
{
    struct ptree root;
    struct ptree *p, *pfind;
    struct ptree_mask *pm;
    uint32_t host;


    memset(&root, 0, sizeof(root));
    root.p_key = 0;
    root.p_left = &root;
    root.p_left = &root;
    root.p_b

}


int main(int argc, char **argv)
{


    return 0;
}
