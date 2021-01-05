#include "../common/definitions.h"

#ifndef RSOCK_HELPER_H
#define RSOCK_HELPER_H
    char* getLocalIp_s(char* device);
    int receivePacket(Rsock_packet* rPacket, int sock_r);
    int clientRequest(int socket, int protocol, char source_ip[IPV4STR_MAX_LEN], char dest_ip[IPV4STR_MAX_LEN], int dest_port, char requestMsg[PAYLOAD_MAX_LEN]);


    // unsigned short csum(char* addr, int len);
    // int send_packet(int sock, int protocol, int src_port, int dst_port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN], char* options);
    // int send_data(int sock, int protocol, int port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN]);
    // void* pthread_handle_request(void *vargp);
#endif