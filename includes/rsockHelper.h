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
    #include <netinet/ip.h>
    #include <netinet/udp.h>
    #include <netinet/tcp.h>

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


#ifndef PACKET_HINTS_STRUCT
#define PACKET_HINTS_STRUCT
    struct packet_hint_pointers {
        char* pSrcIpAddr;
        int src_port;
        char* pDestIpAddr;
        int dest_port;
        struct rsock_packet* pTargetSet; 
        int flag;       //DEF: 0-empty pointer || 1-wait for packet || 2-packet ready || 3-packet overwritten (warning!!)
        int sock;
        char* msg;
    };
#endif


#ifndef RSOCK_STRUCT
#define RSOCK_STRUCT
    #include <netinet/if_ether.h>

    struct rsock_packet{
        struct ethhdr eth;
        struct iphdr ip;
        struct sockaddr_in source;
        struct sockaddr_in dest;
        int protocol;

        union{
            struct tcphdr* tcp;
            struct udphdr* udp;
            int other;
        };

        int payload_len;
        unsigned char* payload;
	    unsigned char* pPacket;
    };
#endif

#ifndef TCPLAB_HELPER_GLOBAL_VAR
#define TCPLAB_HELPER_GLOBAL_VAR
    struct addrinfo addr_settings;
    char source_ip[IPV4STR_MAX_LEN];
    struct packet_hint_pointers focusedAddrses[MAX_CONNECTIONS];
#endif

#ifndef RSOCK_HELPER_H
#define RSOCK_HELPER_H
    int init_socket(); 
    unsigned short csum(char* addr, int len);
    int send_packet(int sock, int protocol, int src_port, int dst_port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN], char* options);
    int send_data(int sock, int protocol, int port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN]);
    char* getLocalIp_s(char* device);
    void* pthread_handle_request(void *vargp);
#endif