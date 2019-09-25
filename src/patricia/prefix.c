
#include "prefix.h"

#include <stdlib.h>
#include <string.h>

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
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define IN_MAX_BITS (sizeof(struct in_addr) * 8)
#define IN6_MAX_BITS (sizeof(struct in6_addr) * 8)

void set_all_bits(void* p, size_t l)
{
    uint8_t* u8 = (uint8_t*)p;
    size_t i;
    for (i = 0; i < l; i++) {
        *u8 = 0xFF;
        u8++;
    }
}

// a1 = a1 & a2
void addr_and(void* a1, void* a2, size_t addrlen)
{
    size_t i;
    uint8_t* u1 = a1;
    uint8_t* u2 = a2;
    for (i = 0; i < addrlen; i++) {
        *u1 = (*u1) & (*u2);
        u1++;
        u2++;
    }
}

// floor the p->mask
static void prefix_floor_mask_v4(struct prefix* p)
{
    struct in_addr* addr = (struct in_addr*)&p->naddr;
    struct in_addr expect = { 0 };
    struct in_addr maskaddr = { 0 };
    int i;

    memcpy(&expect, addr, sizeof(expect));

    uint8_t* cursor = (uint8_t*)&maskaddr;
    for (i = 0; i < p->maskbit / 8; i++) {
        *cursor = 0xFF;
        cursor += 1;
    }
    uint8_t m = p->maskbit % 8;
    if (m != 0) {
        *cursor = 0xFF < (8 - m);
    }
    addr_and(&expect, &maskaddr, sizeof(expect));
    if (!IN_ARE_ADDR_EQUAL(addr, &expect)) {
        memcpy(addr, &expect, sizeof(expect));
        inet_ntop(p->addr_family, addr, p->sin_str, sizeof(p->sin_str));
        snprintf(p->string, sizeof(p->string), "%s/%u", p->sin_str, p->maskbit);
    }
}

static void prefix_floor_mask_v6(struct prefix* p)
{
    struct in6_addr* addr = (struct in6_addr*)&p->naddr;
    struct in6_addr expect = { 0 };
    struct in6_addr maskaddr = { 0 };
    int i;

    memcpy(&expect, addr, sizeof(expect));

    uint8_t* cursor = (uint8_t*)&maskaddr;
    for (i = 0; i < p->maskbit / 8; i++) {
        *cursor = 0xFF;
        cursor += 1;
    }
    uint8_t m = p->maskbit % 8;
    if (m != 0) {
        *cursor = 0xFF < (8 - m);
    }
    addr_and(&expect, &maskaddr, sizeof(expect));
    if (!IN6_ARE_ADDR_EQUAL(addr, &expect)) {
        memcpy(addr, &expect, sizeof(expect));
        inet_ntop(p->addr_family, addr, p->sin_str, sizeof(p->sin_str));
        snprintf(p->string, sizeof(p->string), "%s/%u", p->sin_str, p->maskbit);
    }
}

static void prefix_floor_mask(struct prefix* p)
{
    uint32_t maskh;
    const uint8_t max_mask_bits = p->addr_family == AF_INET ? IN_MAX_BITS : IN6_MAX_BITS;

    if (p->addr_family == AF_INET) {
        prefix_floor_mask_v4(p);
    } else {
        prefix_floor_mask_v6(p);
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
    uint8_t max_mask_bit = 0;
    for (sep = str; (*sep != 0) && (*sep != '/'); sep += 1) {
    }

    if (strstr(str, ":") != NULL) {
        p->addr_family = AF_INET6;
        max_mask_bit = IN6_MAX_BITS;
    } else {
        p->addr_family = AF_INET;
        max_mask_bit = IN_MAX_BITS;
    }

    if (*sep) {
        if (*(sep + 1) == 0) {
            return -1;
        }
        char s[10] = { 0 };
        snprintf(s, sizeof(s), "%s", sep + 1);
        uint64_t number;
        number = (uint64_t)strtol(s, 0, 10);
        if (number <= 0 || number > max_mask_bit) {
            return -1;
        }
        p->maskbit = (uint8_t)number;
    } else {
        p->maskbit = max_mask_bit;
        sep = str + strlen(str);
    }

    snprintf(p->sin_str, sizeof(p->sin_str), "%.*s", (int)(sep - str), str);
    int r;
    r = inet_pton(p->addr_family, p->sin_str, &p->naddr);
    if (r != 1) {
        return -1;
    }
    snprintf(p->string, sizeof(p->string), "%s/%u", p->sin_str, p->maskbit);
    prefix_floor_mask(p);
    return 0;
}

int prefix_fprintf(FILE* f, struct prefix* p)
{
    return fprintf(f, "%s", p->string);
}

// maskbit >0
bool prefix_cmp(struct prefix* p1, struct prefix* p2, uint8_t maskbit)
{
    if (maskbit <= 0) {
        return true;
    }
    uint8_t* addr1 = (uint8_t*)&p1->naddr;
    uint8_t* addr2 = (uint8_t*)&p2->naddr;
    uint8_t div = maskbit / 8;
    if (memcmp(addr1, addr2, div) != 0) {
        return false;
    }
    uint8_t mod = maskbit % 8;
    if (mod == 0) {
        return true;
    }
    uint8_t v = 0xFF << (8 - mod);
    return (addr1[div] & v) == (addr2[div] & v);
}

uint8_t prefix_fst_diff_mask(struct prefix* p1, struct prefix* p2)
{
    uint8_t chk_bits = min(p1->maskbit, p2->maskbit);
    uint8_t i;
    uint8_t j;
    uint8_t diff_bit = 0;
    uint8_t diff_byte = 0;
    uint8_t* addr1 = (uint8_t*)&p1->naddr;
    uint8_t* addr2 = (uint8_t*)&p2->naddr;

    for (i = 0; i * 8 < chk_bits; i++) {
        diff_byte = addr1[i] ^ addr2[i];
        if (diff_byte == 0) {
            // If not find diff bit, will use this default value.
            diff_bit = (i + 1) * 8 + 1;
            continue;
        }
        for (j = 0; j < 8; j++) {
            if (diff_byte & (0x80u >> j)) {
                break;
            }
        }
        diff_bit = i * 8 + j + 1;
        break;
    }
    diff_bit = min(min(p1->maskbit, p2->maskbit) + 1, diff_bit);
    return diff_bit;
}

// see mask bit is 1 or 0
// mask >0 && mask <= max_maskbit
bool prefix_test_bit(struct prefix* p, uint8_t mask)
{
    mask -= 1;
    uint8_t div = mask / 8;
    uint8_t mod = mask % 8;
    uint8_t* addr = (uint8_t*)&p->naddr;
    return addr[div] & (0x80u >> mod);
}
