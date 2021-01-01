
#ifndef RSOCK_HELPER_H
#define RSOCK_HELPER_H
    char* getLocalIp_s(char* device);
    int receivePacket(Rsock_packet* rPacket, int sock_r);
    void* receiveThread(void* vargp);


    // unsigned short csum(char* addr, int len);
    // int send_packet(int sock, int protocol, int src_port, int dst_port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN], char* options);
    // int send_data(int sock, int protocol, int port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN]);
    // void* pthread_handle_request(void *vargp);
#endif