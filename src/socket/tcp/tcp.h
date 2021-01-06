#include "../../common/definitions.h"

#ifndef TCPLAB_TCP_HEADER
#define TCPLAB_TCP_HEADER

    unsigned short validate_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
    unsigned short compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
    unsigned short compute_tcpcrc_checksum(struct iphdr *pIph, unsigned short *ipPayload);

    void* tcpHandleRequest_singleThread(void* vargp);
    int tcp_request_wTimeout(int timeoutSec, int send_socket, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, char* requestMsg, char* finalMsg);
    int tcp_request_singleThread(int send_socket, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, char* requestMsg, char* finalMsg);
#endif