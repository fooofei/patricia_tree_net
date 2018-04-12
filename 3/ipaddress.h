
#ifndef IPADDRESS_H
#define IPADDRESS_H


// convert uint32_t ip to char * , MUST free the out.
// Return 0 on success.
int ipaddr_ntop(uint32_t naddr, char ** out);
int ipaddr_htop(uint32_t haddr, char ** out);

int ipaddr_pton(const char * paddr, uint32_t * out);
int ipaddr_ptoh(const char * paddr, uint32_t * out);

void test_ipaddress();

#endif
