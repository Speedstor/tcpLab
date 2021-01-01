

#ifndef NON_STANDARD_CONSTANTS
#define NON_STANDARD_CONSTANTS
    #define IPV4STR_MAX_LEN 15
    #define PSEUDO_TCP_HDR_LEN 12
    #define NETWORK_INTERFACE_MAX_LEN 16
    #define PAYLOAD_MAX_LEN 1400
    #define MAX_CONNECTIONS 40
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
        int socket;
    } Settings_struct;
#endif