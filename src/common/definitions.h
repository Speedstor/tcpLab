#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>


#ifndef NON_STANDARD_CONSTANTS
#define NON_STANDARD_CONSTANTS
    #define IPV4STR_MAX_LEN 15
    #define PSEUDO_TCP_HDR_LEN 12
    #define NETWORK_INTERFACE_MAX_LEN 16
    #define PAYLOAD_MAX_LEN 1400
    #define MESSAGE_MAX_LEN 65536
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


#ifndef settings_struct
#define settings_struct
    typedef struct Settings_struct {
        int protocol;
        int port;
        int src_port;
        int send_mode;
        int receive_mode;
        char dest_ip[IPV4STR_MAX_LEN];
        char source_ip[IPV4STR_MAX_LEN];
        char network_interface[NETWORK_INTERFACE_MAX_LEN];
        char message[PAYLOAD_MAX_LEN];
        int sendSocket;
        int receiveSocket;
    } Settings_struct;
#endif

#ifndef RAW_SOCKET_PACKET
#define RAW_SOCKET_PACKET
    typedef struct Rsock_packet{
        struct ethhdr* eth;
        struct iphdr* ip;
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

        unsigned char* pBuffer;
    }Rsock_packet;
#endif

#ifndef PACKET_HINTS_STRUCT
#define PACKET_HINTS_STRUCT
    typedef struct Packet_hint_pointers {
        char RemoteIpAddr[IPV4STR_MAX_LEN];
        int remote_port;
        char LocalIpAddr[IPV4STR_MAX_LEN];
        int local_port;
        Rsock_packet* pTargetSet; 
        int flag;       //DEF: 0-empty pointer || 1-wait for packet || 2-packet ready || 3-packet overwritten (warning!!)
        int sock;
        char* msg;
        int isWriting;
    }Packet_hint_pointers;
#endif

#ifndef RECEIVE_THREAD_ARGS
#define RECEIVE_THREAD_ARGS
    typedef struct ReceiveThread_args{
        Packet_hint_pointers (*focusedAddrses) [MAX_CONNECTIONS];
        Settings_struct* settings;
    }ReceiveThread_args;
#endif