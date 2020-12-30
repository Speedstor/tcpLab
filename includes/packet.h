#ifndef NON_STANDARD_CONSTANTS
#define NON_STANDARD_CONSTANTS
    #define IPV4STR_MAX_LEN 15
    #define PSEUDO_TCP_HDR_LEN 12
    #define NETWORK_INTERFACE_MAX_LEN 16
    #define PAYLOAD_MAX_LEN 1400
    #define MAX_CONNECTIONS 40
#endif


#ifndef PACKET_H
#define PACKET_H
    int tcp_packet_send(int sock, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions);  //protocol: 6
    int udp_packet_send(int sock, int src_port, int dst_port, struct addrinfo *dest, char message[PAYLOAD_MAX_LEN]); //protocol: 17
    int tcpcrc_packet_send(int sock, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions);  //protocol: 206
    void tcpNoCheck_packet_send(int sock, int src_port, int dst_port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN], char* pOptions);   //protocol: 207
#endif