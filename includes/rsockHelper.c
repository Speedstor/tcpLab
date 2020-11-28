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

#include "packet.h"
#include "spz.h"
#include "tcpHelper.h"
#include "rsock_recv.h"
#include "post.h"


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

#ifndef TCPLAB_HELPER_GLOBAL_VAR
#define TCPLAB_HELPER_GLOBAL_VAR
    struct addrinfo addr_settings;
    char source_ip[IPV4STR_MAX_LEN];
    struct packet_hint_pointers focusedAddrses[MAX_CONNECTIONS];
#endif

char jsonSubmit[99999];

//TAG: finished
int init_socket() {
    //opening socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        // printf("socket opening error\n");
        return -1;
    }else 
        // printf("Opened Socket fine (1)\n");
    return sock;
}

//TAG: finished
unsigned short csum(char* addr, int len){
    unsigned long sum = 0;

    // len -= 2;
    // sum =  sum + *addr;
    while(len > 1){
        sum =  sum + *addr++;
        len -= 2;
    }

    if(len > 0){
        sum = sum + *addr;
    }

    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    
    return (~sum);
}

//TAG: finished
char* getLocalIp_s(char* device){
    char deviceName[strlen(device)+1];
    strncpy(deviceName, device, strlen(device)+1);

    struct ifaddrs *ifaddr, *ifa;
    int s;
    static char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name, deviceName) == 0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0)
            {
                // printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            freeifaddrs(ifaddr);
            return host;
        }
    }
    return NULL;
}

int send_packet(int sock, int protocol, int src_port, int dst_port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN], char* pOptions){
    //Getting dest and source for a "standardized" ip
    struct addrinfo *dest, *source;
    //get and check destination ip
    int status;
    status = getaddrinfo(dest_ip, NULL, &addr_settings, &dest);
    if (status) {
        printf("Destination ip invalid: %s\n", dest_ip);
        return -1;
    }
    if (dest->ai_family != AF_INET && dest->ai_family != AF_INET6) {
        printf("Destination ip unsupported (IPv4): %s\n", dest_ip);
        return -1;
    }
    //get and check source ip
    status = getaddrinfo(source_ip, NULL, &addr_settings, &source);
    if (status) {
        printf("Source ip invalid: %s\n", source_ip);
        return -1;
    }
    if (dest->ai_family != AF_INET && dest->ai_family != AF_INET6) {
        printf("Source ip unsupported (IPv4): %s\n", source_ip);
        return -1;
    }

    int success;
    switch (protocol){
    case 6: //tcp protocol
        success = tcp_packet_send(sock, src_port, dst_port, dest, source, message, pOptions);
        break;
    case 17: //udp protocol
        success = udp_packet_send(sock, src_port, dst_port, dest, message);
        break;
    case 206: //custom modified tcp protocol
        break;
    case 207: //custom modified tcp protocol
        break;
    case 217: //custom modified udp protocol
        printf("is not and probably will not be implemented\n");
        break;
    }

    return success;
}

int send_data(int sock, int protocol, int port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN]){
    refresh_progressBar();
    
    struct sockaddr_in serv_addr;
    struct rsock_packet recv_packet;
    int ifavl;
    char* finalMsg;
    char debugText[100];

    switch(protocol){
    case 6: //standard tcp protocol
    case 206: //custom tcp protocol
        // sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP //required !!!!

        sock = init_socket();

        //get random open port
        ifavl = -1;
        while(ifavl < 0){
            //check if port open, or get port if port is 0
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;

            serv_addr.sin_port = htons(49152 + (rand() % 16300));
            inet_aton(source_ip, &serv_addr.sin_addr);
            if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0) {
                ifavl = 1;
            }
        }

        sprintf(debugText, "port number %d || addr: %d\n", ntohs(serv_addr.sin_port), serv_addr.sin_addr.s_addr);
        print_progressBar(debugText, 10);

        //add ip and port to prepare listen for packet
        finalMsg = (char *)malloc(147483647); 
        int i;
        for(i=0; i < MAX_CONNECTIONS; i++){
            if(focusedAddrses[i].flag == 0 || focusedAddrses[i].flag == 404) {
                struct packet_hint_pointers buffer = {dest_ip, port, source_ip, ntohs(serv_addr.sin_port), &recv_packet, 1, sock, finalMsg};
                focusedAddrses[i] = buffer;
                if(1){
                    sprintf(debugText, "focusedAddrses[%d]: %s | %d | %s | %d\n", i, (char *) focusedAddrses[i].pDestIpAddr, focusedAddrses[i].dest_port, (char*) focusedAddrses[i].pTargetSet, focusedAddrses[i].flag);
                    print_progressBar(debugText, 13);
                }
                break;
            }else{
                if(i == MAX_CONNECTIONS) {
                    print_progressBar("connection full (%d), please close some connection or wait and try again", 100);
                    return -1;
                }
            }
        }

        //initialize random sequence numbers to keep track in the system
        uint32_t seq = rand() % 1000000000;
        uint32_t ack_seq = 0;
        char tcpOptions[50];

        /** 
         * First Packet  ------
         */
        print_progressBar("initiating tcp handshake................", 20);

        memset(&tcpOptions, '\0', 50);
        sprintf(tcpOptions, "s\t%d\t%d\t", seq, ack_seq);
        send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, "", tcpOptions);

        /** 
         * Recieve Packet  ------
         */        
        print_progressBar("waiting response................", 25);

        while(focusedAddrses[i].flag == 1) msleep(100); //wait for packet

        if(recv_packet.tcp->syn == 1 && recv_packet.tcp->ack == 1){    
            print_progressBar("recieved!! -- finished 3-way handshake -> sending actual request", 30);
        }else if(recv_packet.tcp->rst == 1){
            print_progressBar("acknoledge skipped || lost packet || error in ip", 100);
            return -1;
        }else{
            print_progressBar("unkown flag condition!!", 100);
            return -1;
        }

        /** 
         * Third Packet  ------
         */
        seq = ntohl(recv_packet.tcp->ack_seq);
        ack_seq = ntohl(recv_packet.tcp->seq) + 1; //add one for the syn flag

        memset(&tcpOptions, '\0', 50);
        sprintf(tcpOptions, "pa\t%d\t%d\t", seq, ack_seq);

        send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, message, tcpOptions);

        /** 
         * Recieve Packet  ------
         */
        print_progressBar("waiting for whole data and fin bit...", 70);

        while(focusedAddrses[i].flag != 404) {
            sleep(1);
        }
        
        end_progressBar(0);
        printf("%s\n", focusedAddrses[i].msg);

        focusedAddrses[i].flag = 0;
        free(finalMsg);
        break;
    case 17:
        sock = init_socket();

        ifavl = -1;
        while(ifavl < 0){
            //check if port open, or get port if port is 0
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;

            serv_addr.sin_port = htons(49152 + (rand() % 16300));
            inet_aton(source_ip, &serv_addr.sin_addr);
            if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                if(errno == EADDRINUSE) {
                    // printf("the port is not available. already to other process\n");
                } else {
                    // printf("could not bind to process (%d) %s\n", errno, strerror(errno));
                }
            }else{
                ifavl = 1;
            }
        }

        send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, message, NULL);
        break;
    case 217:
        break;
    }
    return -1;
}

void *pthread_handle_request(void *vargp) {
    refresh_progressBar();
    //need ip and port
    struct packet_hint_pointers* hints;
    hints = vargp;
    char ipAddr[IPV4STR_MAX_LEN];
    strncpy(ipAddr, hints->pSrcIpAddr, IPV4STR_MAX_LEN);
    int clientPort = hints->src_port;


    struct rsock_packet recv_packet;
    char* finalMsg = malloc(999999);
    char tcpOptions[50];
    char debugText[100];

    uint32_t seq = rand() % 1000000000;
    uint32_t ack_seq = ntohl(hints->pTargetSet->tcp->seq) + 1;


    int sock = init_socket();
    //set recieve packet
    int i;
    for(i=0; i < MAX_CONNECTIONS; i++){
        if(focusedAddrses[i].flag == 0 || focusedAddrses[i].flag == 404) {

            struct packet_hint_pointers buffer = {hints->pSrcIpAddr, hints->src_port, hints->pDestIpAddr, hints->dest_port, &recv_packet, 1, sock, finalMsg};
            focusedAddrses[i] = buffer;
            if(1){
                sprintf(debugText, "focusedAddrses[%d]: %s | %d | %s | %d\n", i, (char *) focusedAddrses[i].pSrcIpAddr, focusedAddrses[i].src_port, (char*) focusedAddrses[i].pTargetSet, focusedAddrses[i].flag);
                print_progressBar(debugText, 13);
            }
            break;
        }else{
            if(i == MAX_CONNECTIONS) i = -1;
        }
    }
    if(i == -1) {
        if(1){
            print_progressBar( "connection full (%d), please close some connection or wait and try again", MAX_CONNECTIONS);
        }

        memset(&tcpOptions, '\0', 50);
        sprintf(tcpOptions, "r\t%d\t%d\t", seq, ack_seq);
        send_packet(sock, 6, 8900, clientPort, ipAddr, "", tcpOptions);
        return NULL;
    }

    //send sync ack
    memset(&tcpOptions, '\0', 50);
    sprintf(tcpOptions, "sa\t%d\t%d\t", seq, ack_seq);
    send_packet(sock, 6, 8900, clientPort, ipAddr, "", tcpOptions);

    if(1){
        print_progressBar("sent syn + ack packet", 20);
    }

    //get psh ack and parse data
    while(focusedAddrses[i].flag == 1) msleep(100); //wait for packet

    char* debugText2 = DebugTcpHeader(recv_packet.tcp); 
    printf("%s\n", debugText2);
    free(debugText2);

    if(recv_packet.tcp->psh == 1 && recv_packet.tcp->ack == 1){
        if(1){
            print_progressBar("recieved request details--", 30);
        }
    }else if(recv_packet.tcp->rst == 1){
        if(1){
            
            print_progressBar("acknoledge skipped || lost packet || error in ip", 100);
        }
        return NULL;
    }else{
        if(1){
            
            print_progressBar("unkown flag condition!!", 100);
        }
        return NULL;
    }
        

    //send data
    seq = ntohl(recv_packet.tcp->ack_seq);
    ack_seq = ntohl(recv_packet.tcp->seq) + 1; //add one for the syn flag
    memset(&tcpOptions, '\0', 50);
    sprintf(tcpOptions, "a\t%d\t%d\t", seq, ack_seq);
    send_packet(sock, 6, 8900, clientPort, ipAddr, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Velit laoreet id donec ultrices tincidunt arcu. Sollicitudin aliquam ultrices sagittis orci a scelerisque purus semper. A condimentum vitae sapien pellentesque. Amet cursus sit amet dictum sit. Venenatis tellus in metus vulputate eu scelerisque felis imperdiet. Lectus sit amet est placerat in. Nisl suscipit adipiscing bibendum est."
        , tcpOptions);

    //fin, psh, ack
    memset(&tcpOptions, '\0', 50);
    sprintf(tcpOptions, "fpa\t%d\t%d\t", seq, ack_seq);
    send_packet(sock, 6, 8900, clientPort, ipAddr, "", tcpOptions);
    
    if(1){
        print_progressBar("Finished, sent data", 100);
    }
    
    end_progressBar(0);
    free(hints->pTargetSet->pPacket);
    free(hints->pTargetSet);
    free(finalMsg);
    
    // focusedAddrses[i].flag = 0;

    return NULL;
}
