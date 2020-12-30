// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>

#define CRC16 0x8005

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

//modified from: https://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum
unsigned short compute_tcpcrc_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
    // register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    //initialize checksum in the tcp header to 0
    struct tcphdr* tcphdr = (struct tcphdr*)(ipPayload);
    tcphdr->check = 0;

    //add the pseudo header 
    //combining all the data with the "header header" at the bottom
    char finalData[tcpLen + 6];
    strncpy(finalData, (char *)tcphdr, tcpLen);
    
    char transferBuffer[2];
    int tcpCrcProtocol = 206; //customly defined protocol
    //adding - the source ip -> dest ip -> protocol -> tcpLength
    strncpy(transferBuffer, (char *) &pIph->saddr, 2);
    strncat(finalData, transferBuffer, 2);
    strncpy(transferBuffer, (char *) &pIph->daddr, 2);
    strncat(finalData, transferBuffer, 2);
    uint8_t numberBuffer = htons(tcpCrcProtocol);
    strncpy(transferBuffer, (char *) &numberBuffer, 1);
    strncat(finalData, transferBuffer, 1);
    numberBuffer = htons(tcpLen);
    strncpy(transferBuffer, (char *) &numberBuffer, 1);
    strncat(finalData, transferBuffer, 1);

    int dataLength = tcpLen + 6; //additional 12 8-bits(char) added to the data to be go through crc
    dataLength *= 2; //times 2 because the calculation is incremented by 8 bits, instead of the 16 bits of the char variable that the length is representing

    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    const uint8_t *data = (uint8_t *) &finalData;
    //run the data bits through the CRC loop starting from the least significant bit instead of from the most significant bit
    while(dataLength > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1;

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            dataLength--;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;
    }

    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
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