#ifndef PATRICIA_PREFIX_H
#define PATRICIA_PREFIX_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

struct prefix
{
    char string[16+1+4]; // 255.255.255.255/255
    char sin_str[16];
    uint8_t maskbit;
    uint32_t sin;   // network order
    uint32_t host; // host order
};


#ifdef __cplusplus
extern "C" {
#endif
    int prefix_format(struct prefix* p, const char* str);
    int prefix_fprintf(struct prefix * p, FILE * f);
    bool prefix_cmp(struct prefix * p1, struct prefix * p2);

#ifdef __cplusplus
}
#endif

#endif
