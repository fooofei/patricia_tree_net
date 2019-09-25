#ifndef PATRICIA_PREFIX_H
#define PATRICIA_PREFIX_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#ifdef WIN32
#include <In6addr.h>
#include <Windows.h>
#else

#include <netinet/in.h>

#endif

struct prefix {
    char string[INET6_ADDRSTRLEN + 1 + 4]; // 255.255.255.255/255
        // ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128
    char sin_str[INET6_ADDRSTRLEN];
    uint8_t maskbit; // starts from 1
    uint8_t addr_family; // AF_INET or AF_INET6
    struct in6_addr naddr; // network order
};

int prefix_format(struct prefix* p, const char* str);
int prefix_fprintf(FILE* f, struct prefix* p);
bool prefix_cmp(struct prefix* p1, struct prefix* p2, uint8_t maskbit);
// Find the first different mask of p1 and p2.
// The return value is starts from 1.
uint8_t prefix_fst_diff_mask(struct prefix* p1, struct prefix* p2);
bool prefix_test_bit(struct prefix* p, uint8_t mask);
#endif
