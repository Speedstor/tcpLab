#include "../../common/definitions.h"

#ifndef TCPLAB_TCP_HEADER
#define TCPLAB_TCP_HEADER

    unsigned short validate_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
    unsigned short compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
    unsigned short compute_tcpcrc_checksum(struct iphdr *pIph, unsigned short *ipPayload);

    int tcp_request_singleThread(int socket, int destPort, struct sockaddr_in serv_addr, char dest_ip[IPV4STR_MAX_LEN], char requestMsg[PAYLOAD_MAX_LEN], char* finalMsg, Packet_hint_pointers (* focusedAddrses)[MAX_CONNECTIONS]);
#endif