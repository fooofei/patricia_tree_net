
#ifndef IPADDRESS_H
#define IPADDRESS_H


// convert uint32_t ip to char * , MUST free the out.
void ipaddr_ntop(uint32_t naddr, char ** out);
void ipaddr_htop(uint32_t haddr, char ** out);

void ipaddr_pton(const char * paddr, uint32_t * out);
void ipaddr_ptoh(const char * paddr, uint32_t * out);

void test_ipaddress();

#endif
