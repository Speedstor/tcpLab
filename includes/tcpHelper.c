// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>

unsigned short compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
    register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    //add the pseudo header 
    //the source ip -> dest ip -> protocol -> tcpLength
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    sum += htons(IPPROTO_TCP);
    sum += htons(tcpLen);

    /**
     *  For Debugging, print out tcp packet in hex into console 
    
    int i;
    unsigned char *pointer = (unsigned char *)ipPayload;
    for (i = 0; i < tcpLen; i++) {
        printf("%02x ", pointer[i]);
    }
    printf("\n");
    */

    //initialize checksum to 0
    struct tcphdr* tcphdr = (struct tcphdr*)(ipPayload);
    tcphdr->check = 0;

    //add the IP payload
    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }    
    //if any bytes left, pad the bytes and add
    if(tcpLen > 0) {
        // printf("+++++++++++padding, %dn", tcpLen);
        sum += ((*ipPayload)&htons(0xFF00));
    }

    //turn 32-bit sum to 16-bit: add carrier to result
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;

    return (unsigned short)sum;
}

unsigned short validate_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
    register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    //add the pseudo header 
    //the source ip -> dest ip -> protocol -> tcpLength
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    sum += htons(IPPROTO_TCP);
    sum += htons(tcpLen);

    /**
     *  For Debugging, print out tcp packet in hex into console 
    
    int i;
    unsigned char *pointer = (unsigned char *)ipPayload;
    for (i = 0; i < tcpLen; i++) {
        printf("%02x ", pointer[i]);
    }
    printf("\n");
    */

    //Does not turn checksum to 0 -> to validate the sum will be 0

    //add the IP payload
    // printf("ipPayload csum: %d\n", tcpLen);
    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }    
    //if any bytes left, pad the bytes and add
    if(tcpLen > 0) {
        // printf("+++++++++++padding, %dn", tcpLen);
        sum += ((*ipPayload)&htons(0xFF00));
    }

    //turn 32-bit sum to 16-bit: add carrier to result
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;

    //set computation result
    return (unsigned short) sum;
}