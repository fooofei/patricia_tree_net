

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#ifdef WIN32
#include <Ws2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// If not `#include <arpa/inet.h>`, compile will have a warning:
//   In function ‘ipaddr_ntop’:
//   warning: assignment makes pointer from integer without a cast
// the compile & link is passed, but run will crash
// because the return value is wrong.
//   Expect:
//      (gdb) p &buf[0]
//      $2 = 0x7fffffffe210 "127.0.0.1"
//   But:
//     (gdb) p p
//      $1 = 0xffffffffffffe210 < Address 0xffffffffffffe210 out of bounds >
#endif


#include "ipaddress.h"


struct ipaddress_pair
{
    const char * p;
    uint32_t n;
    uint32_t h;
};

static const struct ipaddress_pair g_test_data[] = {
    {"127.0.0.1",16777343 ,2130706433},
    {"192.145.109.100",1684902336 ,3230756196},
    {"192.168.1.3",50440384 ,3232235779},
    {0,0,0}
};


int ipaddr_ntop(uint32_t naddr, char ** out)
{
    *out = 0;
    enum { _paddr_size = 16, };
    char buf[_paddr_size] = { 0 };
    const char * p;
    p = inet_ntop(AF_INET, &naddr, &buf[0], _paddr_size);
    if (p) {
#ifdef WIN32
        *out = _strdup(p);
#else
        *out = strdup(p);
#endif
        return 0;
    }
    return -1;
}

int ipaddr_htop(uint32_t haddr, char ** out)
{
    *out = 0;
    uint32_t naddr;

    naddr = htonl(haddr);
    return ipaddr_ntop(naddr, out);
}

int ipaddr_pton(const char * paddr, uint32_t * out)
{
    // Windows and linux all return 1 on success.
    int ret;
    ret = inet_pton(AF_INET, paddr, out);
    return (ret == 1) ? 0 : -1;
}

int ipaddr_ptoh(const char * paddr, uint32_t * out)
{
    int ret;
    uint32_t naddr = 0;
    ret = ipaddr_pton(paddr, &naddr);
    *out = ntohl(naddr);
    return ret;
}

int ipaddr_hton(uint32_t haddr, uint32_t * out)
{
    *out = htonl(haddr);
    return 0;
}

int ipaddr_ntoh(uint32_t naddr, uint32_t * out)
{
    *out = ntohl(naddr);
    return 0;
}

// Ref https://github.com/tobez/Net-Patricia/blob/master/libpatricia/patricia.c
int ipaddr_pton2(const char *src, uint32_t *dst)
{
    int i, c, val;
    u_char xp[sizeof(uint32_t)] = { 0, 0, 0, 0 };

    for (i = 0; ; i++)
    {
        c = *src++;
        if (!isdigit(c))
        {
            return (-1);
        }

        val = 0;
        do {
            val = val * 10 + c - '0';
            if (val > 255)
            {
                return (-1);
            }
            c = *src++;
        } while (c && isdigit(c));
        xp[i] = val;
        if (c == '\0')
            break;
        if (c != '.')
            return (-1);
        if (i >= 3)
            return (-1);
    }
    memcpy(dst, xp, sizeof(uint32_t));
    return 0;
}


#define EXPECT(expr) \
    do { \
    if(!(expr)) \
        { \
        fprintf(stderr, "unexpect %s  (%s:%d)\n",#expr, __FILE__, __LINE__); \
        fflush(stderr);\
        } \
    } while (0)

void test_ipaddress()
{
    char * paddr;
    uint32_t naddr;
    uint32_t haddr;

    const struct ipaddress_pair * p;

    for (p = g_test_data; p->p; p += 1)
    {
        paddr = 0;
        naddr = 0;
        haddr = 0;

        ipaddr_ntop(p->n, &paddr);
        EXPECT(0 == strcmp(paddr, p->p));
        free(paddr);

        ipaddr_htop(p->h, &paddr);
        EXPECT(0 == strcmp(paddr, p->p));
        free(paddr);


        ipaddr_ptoh(p->p, &haddr);
        EXPECT(haddr == p->h);

        ipaddr_pton(p->p, &naddr);
        EXPECT(naddr == p->n);
    }
    printf("pass %s()\n", __FUNCTION__);
}


// int main()
// {
//     test_ipaddress();
//     return 0;
// }
