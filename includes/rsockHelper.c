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
        char* payload;
	    char* pPacket;
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

int send_packet(int sock, int protocol, int src_port, int dst_port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN], char* pOptions)
{
    //parse options
    int optionLen;
    if(pOptions != NULL){
        optionLen = strlen(pOptions);
    }else{
        optionLen = 0;
    }
    char options[optionLen];
    strncpy(options, pOptions, optionLen);

    //setting up address ips
    struct addrinfo *dest, *source;

    //get and check destination ip
    int status;
    status = getaddrinfo(dest_ip, NULL, &addr_settings, &dest);
    if (status) {
        // printf("Destination ip invalid: %s\n", dest_ip);
        return -1;
    }
    if (dest->ai_family != AF_INET && dest->ai_family != AF_INET6) {
        // printf("Destination ip unsupported (IPv4): %s\n", dest_ip);
        return -1;
    }

    //get and check source ip
    status = getaddrinfo(source_ip, NULL, &addr_settings, &source);
    if (status) {
        // printf("Source ip invalid: %s\n", source_ip);
        return -1;
    }
    if (dest->ai_family != AF_INET && dest->ai_family != AF_INET6) {
        // printf("Source ip unsupported (IPv4): %s\n", source_ip);
        return -1;
    }

    struct sockaddr_in *sockaddr4;
    struct udp_packet uBufferPacket;
    struct tcp_packet tBufferPacket;
    struct tcphdr* pTcphdr = &tBufferPacket.tcp;
    //struct custom_udp_packet c_uBufferPacket;
    //struct custom_udp_packet c_tBufferPacket;

    //calculate packet size
    int headersize;
    int packetsize;
    int tcpHeaderLen;
    int seq; 
    int seq_ack; 

    int success;

    switch (protocol)
    {
    case 6: //tcp protocol
        //construct header
        tcpHeaderLen = 20;
        headersize = sizeof(tBufferPacket.ip) + tcpHeaderLen;
        packetsize = headersize + strlen(message);

        // printf("message len: %ld || packet size: %d \n", strlen(message), packetsize);

        tBufferPacket.ip.ip_v = 4;
        tBufferPacket.ip.ip_hl = sizeof(tBufferPacket.ip) >> 2;
        tBufferPacket.ip.ip_dst = ((struct sockaddr_in *) dest->ai_addr)->sin_addr;
        tBufferPacket.ip.ip_src = ((struct sockaddr_in *) source->ai_addr)->sin_addr;
        //source is set automatically in linux || but not because we need it for check sum, can't calculuate with incomplete header
        tBufferPacket.ip.ip_p = 6;
        tBufferPacket.ip.ip_ttl = 23;
        tBufferPacket.ip.ip_len = htons(packetsize);

        tBufferPacket.tcp.source = htons(src_port);
        tBufferPacket.tcp.dest = htons(dst_port);
        tBufferPacket.tcp.doff = tcpHeaderLen/4;
        tBufferPacket.tcp.res1 = 0;
        tBufferPacket.tcp.res2 = 0;
        
        //seperate string for char options
        char *pParam = strchr(options, '\t');
        if (pParam != NULL)
        {
            memset(pParam, '\0', 1);
        }

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
            // char secParam[]
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

        /**
         * Debugging the hexdump of the about to be send tcp
        */
        // int ia;
        // unsigned char *pointer1 = (unsigned char *)&tBufferPacket;
        // for (ia = 0; ia < packetsize; ia++) {
        //     printf("%02x ", pointer1[ia]);
        // }
        // printf("\n");
        

        //send packet
        // printf("Sending tcp packet .....................\n");
        success = sendto(sock, &tBufferPacket, packetsize, 0, dest->ai_addr, dest->ai_addrlen);


        // char jsonSubmit[99999];
        sprintf(jsonSubmit, "{\"tableName\": \"packet_sent\", \"seq\": %d, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", htonl(seq), toBinaryString((void *) message, strlen(message)), toBinaryString((void *) &tBufferPacket, packetsize), getTimestamp());
        async_db_put((void *) jsonSubmit, 1);

        if (success < 0)
        {
            // printf("ERROR SENDING PACKET\n");
            return -1;
        }
        break;
    case 206: //custom modified tcp protocol
        break;
    case 17: //udp protocol
        //construct header
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
        uBufferPacket.udp.uh_sum = 0; //linux handles automatically  //TODO: make own checksum

        memset(uBufferPacket.payload, '\0', PAYLOAD_MAX_LEN);
        strcpy(uBufferPacket.payload, message);

        //send packet
        // printf("Sending packet .....................\n");
        success = sendto(sock, &uBufferPacket, packetsize, 0, dest->ai_addr, dest->ai_addrlen);
        if (success < 0) {
            // printf("ERROR SENDING PACKET\n");
            return -1;
        }
        break;
    case 217: //custom modified udp protocol
        // printf("is not implemented and probably will not be\n");
        break;
    }

    return 1;
}

int send_data(int sock, int protocol, int port, char dest_ip[IPV4STR_MAX_LEN], char message[PAYLOAD_MAX_LEN]){
    refresh_progressBar();
    // printf("\n");
    struct sockaddr_in serv_addr;
    struct rsock_packet recv_packet;
    socklen_t len = sizeof(serv_addr);
    int ifavl;
    char* finalMsg;
    char debugText[100];

    switch(protocol){
    case 6:
        sock = init_socket();
        // sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP //required !!!!

        ifavl = -1;
        while(ifavl < 0){
            //check if port open, or get port if port is 0
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            // serv_addr.sin_addr.s_addr = INADDR_ANY;

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
        if(1){
            
            sprintf(debugText, "port number %d || addr: %d\n", ntohs(serv_addr.sin_port), serv_addr.sin_addr.s_addr);
            print_progressBar(debugText, 10);
        }

        //add ip and port to prepare listen for packet
        // struct packet_hint_pointers;
        finalMsg = (char *)malloc(147483647 /* padding */);
        int i;
        for(i=0; i < MAX_CONNECTIONS; i++){
            if(focusedAddrses[i].flag == 0) {

                struct packet_hint_pointers buffer = {dest_ip, port, source_ip, ntohs(serv_addr.sin_port), &recv_packet, 1, sock, finalMsg};
                focusedAddrses[i] = buffer;
                if(1){
                    
                    sprintf(debugText, "focusedAddrses[%d]: %s | %d | %s | %d\n", i, (char *) focusedAddrses[i].pDestIpAddr, focusedAddrses[i].dest_port, (char*) focusedAddrses[i].pTargetSet, focusedAddrses[i].flag);
                    print_progressBar(debugText, 13);
                }
                break;
            }else{
                if(i == MAX_CONNECTIONS) i = -1;
            }
        }
        if(i == -1) {
            if(1){
                sprintf(debugText, "connection full (%d), please close some connection or wait and try again", MAX_CONNECTIONS);
                print_progressBar(debugText, 100);
            }
            // printf("connection full (%d), please close some connection or wait and try again", MAX_CONNECTIONS);
            return -1;
        }

        uint32_t seq = rand() % 100000;
        uint32_t ack_seq = 0;
        char tcpOptions[50];

        /** 
         * First Packet  ------
         */
        if(1){
            sprintf(debugText, "initiating tcp handshake................");
            print_progressBar(debugText, 20);
        }
        memset(&tcpOptions, '\0', 50);
        sprintf(tcpOptions, "s\t%d\t%d\t", seq, ack_seq);
        send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, "", tcpOptions);

        /** 
         * Recieve Packet  ------
         */        
        if(1){
            sprintf(debugText, "waiting response................");
            print_progressBar(debugText, 25);
        }
        while(focusedAddrses[i].flag == 1) msleep(100); //wait for packet

        if(recv_packet.tcp->syn == 1 && recv_packet.tcp->ack == 1){
            // printf("recieved !! -----------------------------------------------  \n");        
            if(1){
                
                sprintf(debugText, "recieved!! -- finished 3-way handshake -> sending actual request");
                print_progressBar(debugText, 30);
            }
        }else if(recv_packet.tcp->rst == 1){
            if(1){
                sprintf(debugText, "acknoledge skipped || lost packet || error in ip");
                print_progressBar(debugText, 100);
            }
            // printf("acknoledge skipped || lost packet || error in ip");
            return -1;
        }else{
            if(1){
                
                sprintf(debugText, "unkown flag condition!!");
                print_progressBar(debugText, 100);
            }
            // printf("unkown flag condition!!");
            return -1;
        }
        

        // /** 
        //  * Second Packet  ------
        //  */
        // seq = ntohl(recv_packet.tcp->ack_seq);
        // ack_seq = ntohl(recv_packet.tcp->seq) + 1; //add one for the syn flag

        // memset(&tcpOptions, '\0', 50);
        // sprintf(tcpOptions, "a\t%d\t%d\t", seq, ack_seq);
        // send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, "", tcpOptions);

        //finish 3-way handshake

        /** 
         * Third Packet  ------
         */
        seq = ntohl(recv_packet.tcp->ack_seq);
        ack_seq = ntohl(recv_packet.tcp->seq) + 1; //add one for the syn flag


        // memset(&tcpOptions, '\0', 50);
        // sprintf(tcpOptions, "a\t%d\t%d\t", seq, ack_seq);
        // send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, "", tcpOptions);


        memset(&tcpOptions, '\0', 50);
        sprintf(tcpOptions, "pa\t%d\t%d\t", seq, ack_seq);

        focusedAddrses[i].flag = 1;

        send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, message, tcpOptions);

        /** 
         * Recieve Packet  ------
         */
        if(1){
            sprintf(debugText, "waiting for whole data and fin bit...");
            print_progressBar(debugText, 70);
        }

        while(focusedAddrses[i].flag != 404) {
            sleep(1);
        }
        
        end_progressBar(0);
        // printf("%s", DebugTcpHeader(recv_packet.tcp));
        printf("%s\n", focusedAddrses[i].msg);

        focusedAddrses[i].flag = 0;


        break;
    case 17:
        sock = init_socket();

        ifavl = -1;
        while(ifavl < 0){
            //check if port open, or get port if port is 0
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            // serv_addr.sin_addr.s_addr = INADDR_ANY;

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

        // printf("port number %d\n", ntohs(serv_addr.sin_port));

        send_packet(sock, protocol, ntohs(serv_addr.sin_port), port, dest_ip, message, NULL);
        break;
    case 206:
        break;
    case 217:
        break;
    }
}


struct rsock_packet* raw_recv(char* pIpAddr, int localport){

    //get ipAddr literal
    char ipAddr[strlen(pIpAddr)+1];
    strncpy(ipAddr, pIpAddr, strlen(pIpAddr) + 1);

	int sock_r = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sock_r < 0) {
		// printf("error in opening socket\n");
		return NULL;
	}else {
        // // printf("socket opened fine\n");
    }

	while(1){
		struct rsock_packet* recieve = recievePacket(sock_r);

        if(strcmp(ipAddr, inet_ntoa((recieve->source).sin_addr)) == 0 && recieve->tcp->dest == localport) {
            if(validate_tcp_checksum(&recieve->ip, (unsigned short *) recieve->tcp) != 0){
                // printf("ERROR: !!recvieved tcp packet checksum invalid\n");
                return NULL;
            }

            //debug to screen
            // // printf("%s", DebugEthernetHeader((struct ethhdr*) recieve));
            // // printf("%s\n\n", DebugIPHeader((struct iphdr*) (&recieve->ip), recieve->source, recieve->dest));
            // char* pointer = (char *) recieve->tcp; //--------------===
            // // printf("len: %d\n", recieve->tcp->doff * 4);
            // for(int i = 0; i < recieve->tcp->doff * 4; i++){
            // //     printf("%02x ", (unsigned char) pointer[i]);
            // }
            // // printf("\n");                          //--------------===
            // // printf("%s", DebugTcpHeader(recieve->tcp));
            // //printf("%s", DebugUdpHeader(udp));

            return recieve;
        }
    }
}

char* getLocalIp_s(char* device){
    char deviceName[strlen(device)+1];
    strncpy(deviceName, device, strlen(device)+1);

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
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


void *pthread_handle_request(void *vargp) {
    refresh_progressBar();
    //need ip and port
    struct packet_hint_pointers* hints;
    hints = vargp;
    char ipAddr[IPV4STR_MAX_LEN];
    strncpy(ipAddr, hints->pSrcIpAddr, IPV4STR_MAX_LEN);
    int clientPort = hints->src_port;


    struct sockaddr_in serv_addr;
    struct rsock_packet recv_packet;
    socklen_t len = sizeof(serv_addr);
    char* finalMsg;
    char tcpOptions[50];
    char debugText[100];

    uint32_t seq = rand() % 100000;
    uint32_t ack_seq = hints->pTargetSet->tcp->seq + 1;


    int sock = init_socket();
    //set recieve packet
    int i;
    for(i=0; i < MAX_CONNECTIONS; i++){
        if(focusedAddrses[i].flag == 0) {

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
            
            sprintf(debugText, "connection full (%d), please close some connection or wait and try again", MAX_CONNECTIONS);
            print_progressBar(debugText, 100);
        }
        // printf("connection full (%d), please close some connection or wait and try again", MAX_CONNECTIONS);        
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
        
        sprintf(debugText, "sent syn + ack packet");
        print_progressBar(debugText, 20);
    }

    //get psh ack and parse data
    while(focusedAddrses[i].flag == 1) msleep(100); //wait for packet

    // printf("%s\n", DebugTcpHeader(recv_packet.tcp));

    if(recv_packet.tcp->psh == 1 && recv_packet.tcp->ack == 1){
        // printf("recieved !! -----------------------------------------------  \n");        
        if(1){
            
            sprintf(debugText, "recieved request details--");
            print_progressBar(debugText, 30);
        }
    }else if(recv_packet.tcp->rst == 1){
        if(1){
            
            sprintf(debugText, "acknoledge skipped || lost packet || error in ip");
            print_progressBar(debugText, 100);
        }
        // printf("acknoledge skipped || lost packet || error in ip");
        return NULL;
    }else{
        if(1){
            
            sprintf(debugText, "unkown flag condition!!");
            print_progressBar(debugText, 100);
        }
        // printf("unkown flag condition!!");
        return NULL;
    }
        

    //send data
    seq = ntohl(recv_packet.tcp->ack_seq);
    ack_seq = ntohl(recv_packet.tcp->seq) + 1; //add one for the syn flag
    memset(&tcpOptions, '\0', 50);
    sprintf(tcpOptions, "a\t%d\t%d\t", seq, ack_seq);
    send_packet(sock, 6, 8900, clientPort, ipAddr, "\
HTTP/1.1 200 OK\r\n\
Server: SpeedstorServer/0.0.1\r\n\
Content-Length: 35\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\
\r\n\
<h1>My Home page</h1>", tcpOptions);

    //fin, psh, ack
    memset(&tcpOptions, '\0', 50);
    sprintf(tcpOptions, "fpa\t%d\t%d\t", seq, ack_seq);
    send_packet(sock, 6, 8900, clientPort, ipAddr, "", tcpOptions);
    
    if(1){
        
        sprintf(debugText, "Finished, sent data");
        print_progressBar(debugText, 100);
    }
    
    focusedAddrses[i].flag = 0;
    end_progressBar(0);
}
