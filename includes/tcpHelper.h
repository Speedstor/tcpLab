#ifndef PSEUDO_TCP_HEADER
#define PSEUDO_TCP_HEADER
    struct pseudoTcpHeader {
        unsigned int ip_src;
        unsigned int ip_dst;
        unsigned char zero;//always zero
        unsigned char protocol;// = 6;//for tcp
        unsigned short tcp_len;
    };
#endif

#ifndef TCP_HELPER_H
#define TCP_HELPER_H
    unsigned short compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
    unsigned short validate_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
#endif