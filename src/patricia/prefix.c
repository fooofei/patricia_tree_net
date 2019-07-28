
#include "prefix.h"

#include <string.h>
#include <stdlib.h>


#ifdef WIN32
#include <Ws2tcpip.h>
#include <winsock.h>
#else
#include <arpa/inet.h>
// endian.h locate where?
//#include <machine/endian.h>
#include <sys/types.h>
#endif


#ifndef max
#define max(a,b) (((a) (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


// convert mask 8 -> 0xFF 00 00 00
static uint32_t maskbit2host(uint8_t maskbit)
{
    const uint8_t max_mask = sizeof(uint32_t) * 8;
    uint8_t mask;
    uint32_t v;

    mask = min(max_mask, maskbit);
    v = 0x01;
    v = (v << (max_mask - mask)) - 1;
    v = ~v;
    return v;
}

// floor the p->mask
static void prefix_floor_mask(struct prefix* p)
{
    uint32_t h;
    uint32_t maskh;
    const uint8_t max_mask = sizeof(uint32_t) * 8;

    maskh = maskbit2host(p->maskbit);
    h = p->host;
    h = h & maskh;
    if (h != p->host) {
        p->host = h;
        p->sin = htonl(p->host);
        p->maskbit = min(max_mask, p->maskbit);
        inet_ntop(AF_INET, &p->sin, p->sin_str, sizeof(p->sin_str));
    }
}

/* Convert 127.0.0.1/32 to network order prefix struct.
    If only give 127.0.0.1, then the mask default is 32.
  依据掩码做格式化，比如  10.1.0.0/9，被格式化为  10.0.0.0/9.

  Return 0 on success.
*/
int prefix_format(struct prefix* p, const char* str)
{
    const char* sep;

    for (sep = str; (*sep != 0) && (*sep != '/'); sep += 1) {

    }

    if (*sep) {
        if (*(sep+1) == 0) {
            return -1;
        }
        char s[10] = { 0 };
        snprintf(s, sizeof(s), "%s", sep+1);
        uint64_t number;
        number = (uint64_t)strtol(s, 0, 10);
        if (number <=0 || number > sizeof(struct in_addr)*8) {
            return -1;
        }
        p->maskbit = (uint8_t)number;
    } else {
        p->maskbit = 32;
        sep = str + strlen(str);
    }

    snprintf(p->sin_str, sizeof(p->sin_str), "%.*s", (int)(sep - str), str);
    inet_pton(AF_INET, p->sin_str, &p->sin);
    p->host = ntohl(p->sin);
    prefix_floor_mask(p);
    snprintf(p->string, sizeof(p->string), "%s/%u", p->sin_str, p->maskbit);
    return 0;
}

int prefix_fprintf(FILE * f, struct prefix* p)
{
    return fprintf(f, "%s", p->string);
}

// maskbit >0
bool prefix_cmp(struct prefix* p1, struct prefix* p2, uint8_t maskbit)
{
    uint32_t h1 = p1->host;
    uint32_t h2 = p2->host;
    union {
        struct {
            // on Windows always be little-endian
#ifdef WIN32
            uint32_t masklow;
            uint32_t maskhigh;
#else
#if BYTE_ORDER == BIG_ENDIAN
            uint32_t maskhigh;
            uint32_t masklow;
#else
            uint32_t masklow;
            uint32_t maskhigh;
#endif
#endif
        };
        uint64_t mask64;
    }masku;
    memset(&masku, 0, sizeof(masku));
    uint32_t mask;
    masku.maskhigh = 0xFFFFFFFFu;
    masku.mask64 = masku.mask64 >> maskbit;
    // masklow = ~maskhigh
    mask = masku.masklow;
    h1 = h1 & mask;
    h2 = h2 & mask;
    return h1 == h2;
}
