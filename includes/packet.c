#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h> 

#ifndef NON_STANDARD_CONSTANTS
#define NON_STANDARD_CONSTANTS
    #define IPV4STR_MAX_LEN 15
    #define PSEUDO_TCP_HDR_LEN 12
    #define NETWORK_INTERFACE_MAX_LEN 16
    #define PAYLOAD_MAX_LEN 1400
    #define MAX_CONNECTIONS 40
#endif

#ifndef JUMBO_PACKET_HELPER
#define JUMBO_PACKET_HELPER
    struct udp_packet {
        struct ip ip;
        struct udphdr udp;
        char payload[PAYLOAD_MAX_LEN];
    } __attribute__((packed));

    struct tcp_packet {
        struct ip ip;
        struct tcphdr tcp;
        char payload[PAYLOAD_MAX_LEN];
    } __attribute__((packed));
#endif


void tcpNoCheck_packet_send(int sock, int src_port, int dst_port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN], char* pOptions);   //protocol: 207

void tcpcrc_packet_send(int sock, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions){
  //construct header
    int protocol = 6;
    char jsonSubmit[99999];

    int headersize;
    int packetsize;
    int tcpHeaderLen;
    uint32_t seq; 
    uint32_t seq_ack; 

    struct tcp_packet tBufferPacket;

    int success;

    //parse options
    //TODO: comment and explain cuz its not standard procedure
    int optionLen;
    if(pOptions != NULL) optionLen = strlen(pOptions);
    else optionLen = 0;

    char options[optionLen];
    strncpy(options, pOptions, optionLen);
    //seperate string for char options
    char *pParam = strchr(options, '\t');
    if (pParam != NULL) memset(pParam, '\0', 1);

    tcpHeaderLen = 20;
    headersize = sizeof(tBufferPacket.ip) + tcpHeaderLen;
    packetsize = headersize + strlen(message);

    // printf("message len: %ld || packet size: %d \n", strlen(message), packetsize);

    tBufferPacket.ip.ip_v = 4;
    tBufferPacket.ip.ip_hl = sizeof(tBufferPacket.ip) >> 2;
    tBufferPacket.ip.ip_dst = ((struct sockaddr_in *) dest->ai_addr)->sin_addr;
    tBufferPacket.ip.ip_src = ((struct sockaddr_in *) source->ai_addr)->sin_addr;
    tBufferPacket.ip.ip_p = protocol;
    tBufferPacket.ip.ip_ttl = 23;
    tBufferPacket.ip.ip_len = htons(packetsize);

    tBufferPacket.tcp.source = htons(src_port);
    tBufferPacket.tcp.dest = htons(dst_port);
    tBufferPacket.tcp.doff = tcpHeaderLen/4;
    tBufferPacket.tcp.res1 = 0;
    tBufferPacket.tcp.res2 = 0;
    
    //options
    if(strchr(pOptions, 'f') != NULL) tBufferPacket.tcp.fin = 1;
    else tBufferPacket.tcp.fin = 0;
    if(strchr(pOptions, 's') != NULL) tBufferPacket.tcp.syn = 1;
    else tBufferPacket.tcp.syn = 0;
    if(strchr(pOptions, 'r') != NULL) tBufferPacket.tcp.rst = 1;
    else tBufferPacket.tcp.rst = 0;
    if(strchr(pOptions, 'p') != NULL) tBufferPacket.tcp.psh = 1;
    else tBufferPacket.tcp.psh = 0;
    if(strchr(pOptions, 'a') != NULL) tBufferPacket.tcp.ack = 1;
    else tBufferPacket.tcp.ack = 0;
    if(strchr(pOptions, 'u') != NULL) tBufferPacket.tcp.urg = 1;
    else tBufferPacket.tcp.urg = 0;

    //rest of options
    seq = 1;
    seq_ack = 0;
    if(pParam != NULL){
        printf("%s\n", pParam);
        char *pSecParam = strchr(pParam + 1, '\t');
        if(pSecParam != NULL){
            memset(pSecParam, '\0', 1);
            seq = strtol(pParam + 1, NULL, 10);
            char *pThirdParam = strchr(pSecParam + 1, '\t');
            if(pThirdParam != NULL){
                memset(pThirdParam, '\0', 1);
                seq_ack = strtol(pSecParam + 1, NULL, 10);
            }
        }
    }
    // printf("seq: %d || seq_ack: %d\n", seq, seq_ack); //for debuging use

    tBufferPacket.tcp.seq = htonl(seq);
    tBufferPacket.tcp.ack_seq = htonl(seq_ack);
    tBufferPacket.tcp.window = htons(65535);
    tBufferPacket.tcp.check = 0;
    tBufferPacket.tcp.urg_ptr = 0;

    memset(tBufferPacket.payload, '\0', PAYLOAD_MAX_LEN);
    strcpy(tBufferPacket.payload, message);

    tBufferPacket.tcp.check = compute_tcpcrc_checksum((struct iphdr *) &tBufferPacket.ip, (unsigned short *) &tBufferPacket.tcp); 
    // printf("validate: %d\n", validate_tcp_checksum((struct iphdr *) &tBufferPacket.ip, (unsigned short *) &tBufferPacket.tcp));

    //send packet
    // printf("Sending tcp packet .....................\n");
    success = sendto(sock, &tBufferPacket, packetsize, 0, dest->ai_addr, dest->ai_addrlen);
    if (success < 0){
        printf("ERROR SENDING PACKET\n");
        return -1;
    }
    
    //record to database
    char* packetBinSeq = toBinaryString((void *) &tBufferPacket, packetsize);
    char* dataBinSeq = toBinaryString((void *) message, strlen(message));
    char* timestamp = getTimestamp();

    sprintf(jsonSubmit, "{\"tableName\": \"packet_sent\", \"seq\": %u, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", seq, dataBinSeq, packetBinSeq, timestamp);
    async_db_put((void *) jsonSubmit, 1);

    free(packetBinSeq);
    free(dataBinSeq);
    free(timestamp);

    return success;   
}

//finished
int udp_packet_send(int sock, int src_port, int dst_port, struct addrinfo *dest, char message[PAYLOAD_MAX_LEN]){
    //construct headers
    int protocol = 17;
    int success;

    struct udp_packet uBufferPacket;
    int headersize;
    int packetsize;

    headersize = sizeof(uBufferPacket.ip) + sizeof(uBufferPacket.udp);
    packetsize = headersize + strlen(message);

    uBufferPacket.ip.ip_v = 4;
    uBufferPacket.ip.ip_hl = sizeof(uBufferPacket.ip) >> 2;
    uBufferPacket.ip.ip_dst = ((struct sockaddr_in *) dest->ai_addr)->sin_addr;
    //source is set automatically in linux
    uBufferPacket.ip.ip_p = protocol;
    uBufferPacket.ip.ip_ttl = 23;
    uBufferPacket.ip.ip_len = htons(packetsize);

    uBufferPacket.udp.uh_dport = htons(src_port);
    uBufferPacket.udp.uh_sport = htons(dst_port);
    uBufferPacket.udp.uh_ulen = htons(strlen(message) + sizeof(uBufferPacket.udp));
    uBufferPacket.udp.uh_sum = 0; //linux handles automatically  //TODO, nah: make own checksum

    memset(uBufferPacket.payload, '\0', PAYLOAD_MAX_LEN);
    strcpy(uBufferPacket.payload, message);

    success = sendto(sock, &uBufferPacket, packetsize, 0, dest->ai_addr, dest->ai_addrlen);
    return success;
}

//finished
int tcp_packet_send(int sock, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions){
    //construct header
    int protocol = 6;
    char jsonSubmit[99999];

    int headersize;
    int packetsize;
    int tcpHeaderLen;
    uint32_t seq; 
    uint32_t seq_ack; 

    struct tcp_packet tBufferPacket;

    int success;

    //parse options
    //TODO: comment and explain cuz its not standard procedure
    int optionLen;
    if(pOptions != NULL) optionLen = strlen(pOptions);
    else optionLen = 0;

    char options[optionLen];
    strncpy(options, pOptions, optionLen);
    //seperate string for char options
    char *pParam = strchr(options, '\t');
    if (pParam != NULL) memset(pParam, '\0', 1);

    tcpHeaderLen = 20;
    headersize = sizeof(tBufferPacket.ip) + tcpHeaderLen;
    packetsize = headersize + strlen(message);

    // printf("message len: %ld || packet size: %d \n", strlen(message), packetsize);

    tBufferPacket.ip.ip_v = 4;
    tBufferPacket.ip.ip_hl = sizeof(tBufferPacket.ip) >> 2;
    tBufferPacket.ip.ip_dst = ((struct sockaddr_in *) dest->ai_addr)->sin_addr;
    tBufferPacket.ip.ip_src = ((struct sockaddr_in *) source->ai_addr)->sin_addr;
    tBufferPacket.ip.ip_p = protocol;
    tBufferPacket.ip.ip_ttl = 23;
    tBufferPacket.ip.ip_len = htons(packetsize);

    tBufferPacket.tcp.source = htons(src_port);
    tBufferPacket.tcp.dest = htons(dst_port);
    tBufferPacket.tcp.doff = tcpHeaderLen/4;
    tBufferPacket.tcp.res1 = 0;
    tBufferPacket.tcp.res2 = 0;
    
    //options
    if(strchr(pOptions, 'f') != NULL) tBufferPacket.tcp.fin = 1;
    else tBufferPacket.tcp.fin = 0;
    if(strchr(pOptions, 's') != NULL) tBufferPacket.tcp.syn = 1;
    else tBufferPacket.tcp.syn = 0;
    if(strchr(pOptions, 'r') != NULL) tBufferPacket.tcp.rst = 1;
    else tBufferPacket.tcp.rst = 0;
    if(strchr(pOptions, 'p') != NULL) tBufferPacket.tcp.psh = 1;
    else tBufferPacket.tcp.psh = 0;
    if(strchr(pOptions, 'a') != NULL) tBufferPacket.tcp.ack = 1;
    else tBufferPacket.tcp.ack = 0;
    if(strchr(pOptions, 'u') != NULL) tBufferPacket.tcp.urg = 1;
    else tBufferPacket.tcp.urg = 0;

    //rest of options
    seq = 1;
    seq_ack = 0;
    if(pParam != NULL){
        printf("%s\n", pParam);
        char *pSecParam = strchr(pParam + 1, '\t');
        if(pSecParam != NULL){
            memset(pSecParam, '\0', 1);
            seq = strtol(pParam + 1, NULL, 10);
            char *pThirdParam = strchr(pSecParam + 1, '\t');
            if(pThirdParam != NULL){
                memset(pThirdParam, '\0', 1);
                seq_ack = strtol(pSecParam + 1, NULL, 10);
            }
        }
    }
    // printf("seq: %d || seq_ack: %d\n", seq, seq_ack); //for debuging use

    tBufferPacket.tcp.seq = htonl(seq);
    tBufferPacket.tcp.ack_seq = htonl(seq_ack);
    tBufferPacket.tcp.window = htons(65535);
    tBufferPacket.tcp.check = 0;
    tBufferPacket.tcp.urg_ptr = 0;

    memset(tBufferPacket.payload, '\0', PAYLOAD_MAX_LEN);
    strcpy(tBufferPacket.payload, message);

    tBufferPacket.tcp.check = compute_tcp_checksum((struct iphdr *) &tBufferPacket.ip, (unsigned short *) &tBufferPacket.tcp); 
    // printf("validate: %d\n", validate_tcp_checksum((struct iphdr *) &tBufferPacket.ip, (unsigned short *) &tBufferPacket.tcp));

    //send packet
    // printf("Sending tcp packet .....................\n");
    success = sendto(sock, &tBufferPacket, packetsize, 0, dest->ai_addr, dest->ai_addrlen);
    if (success < 0){
        printf("ERROR SENDING PACKET\n");
        return -1;
    }
    
    //record to database
    char* packetBinSeq = toBinaryString((void *) &tBufferPacket, packetsize);
    char* dataBinSeq = toBinaryString((void *) message, strlen(message));
    char* timestamp = getTimestamp();

    sprintf(jsonSubmit, "{\"tableName\": \"packet_sent\", \"seq\": %u, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", seq, dataBinSeq, packetBinSeq, timestamp);
    async_db_put((void *) jsonSubmit, 1);

    free(packetBinSeq);
    free(dataBinSeq);
    free(timestamp);

    return success;
}