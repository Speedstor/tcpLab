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

#include "../common/definitions.h"
#include "./tcp/tcp.h"

int receivePacket(Rsock_packet* rPacket, int sock_r);
int clientRequest(int socket, int protocol, char source_ip[IPV4STR_MAX_LEN], char dest_ip[IPV4STR_MAX_LEN], int dest_port, char requestMsg[PAYLOAD_MAX_LEN]);

char* getLocalIp_s(char* device){
    struct ifaddrs *ifaddr, *ifa;
    int s;
    static char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return NULL;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        s = getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name, device) == 0)&&(ifa->ifa_addr->sa_family==AF_INET)) {
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(-1);
            }
            freeifaddrs(ifaddr);
            return host;
        }
    }
    return NULL;
}

int clientRequest(int socket, int protocol, char source_ip[IPV4STR_MAX_LEN], char dest_ip[IPV4STR_MAX_LEN], int dest_port, char requestMsg[PAYLOAD_MAX_LEN]){
    struct sockaddr_in serv_addr;
    char finalMsg[MESSAGE_MAX_LEN];

    //check if port open, or get available free port if port is 0
    int ifavl = -1;
    while(ifavl < 0){
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(49152 + (rand() % 16300));
        inet_aton(source_ip, &serv_addr.sin_addr);

        if(bind(socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)  ifavl = 1;
    }
// int tcp_request(int socket, int destPort, struct sockaddr_in serv_addr, char dest_ip[IPV4STR_MAX_LEN], char requestMsg[PAYLOAD_MAX_LEN], char* finalMsg, Packet_hint_pointers (* focusedAddrses)[MAX_CONNECTIONS]){

    switch(protocol){
        case 6:                                        //standard tcp
// int tcp_request_singleThread(int send_socket, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, char requestMsg[PAYLOAD_MAX_LEN], char* finalMsg){
            tcp_request_singleThread(socket, dest_ip, dest_port, source_ip, ntohs(serv_addr.sin_port), requestMsg, (char *) &finalMsg);
            break;
        case 206:                                      //custom tcp
            break;
        case 7:                                        //standard udp
            break;
        case 207:                                      //custom udp
            return -1;
            break;
    }

    if(strlen(finalMsg) > 0)    printf("Received:  %s\n", finalMsg);
    return 1;
}

int receivePacket(Rsock_packet* rPacket, int sock_r){
	//reception of the network packet || receive the network packet
	unsigned char *buffer = (unsigned char *) malloc(65536); //max buffer memory size
	memset(buffer, 0, 65536);

	//receive a network packet and copy in to buffer
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	int buflen = recvfrom(sock_r, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);
	if(buflen < 0) {
        free(buffer);
		return -1;
	}

	//extract ethernet header                                   ETHERNET
    memcpy(&rPacket->eth, buffer, sizeof(struct ethhdr));

	//extract ip header & get ips of source and destination     IP
    memcpy(&rPacket->ip, (buffer + sizeof(struct ethhdr)), sizeof(struct iphdr));
	struct sockaddr_in source, dest;
	source.sin_addr.s_addr = rPacket->ip.saddr;
	dest.sin_addr.s_addr = rPacket->ip.daddr;

	rPacket->source = source;
	rPacket->dest = dest;
	rPacket->protocol = rPacket->ip.protocol;

	//extract transport layer header                            TRANSPORT
	int iphdrlen = rPacket->ip.ihl * 4;// getting size of IP header from header (more accurate/foolproof)
	switch(rPacket->protocol){
		case 6:
			//tcp
            memcpy(&rPacket->tcp, (buffer + iphdrlen + sizeof(struct ethhdr)), sizeof(struct tcphdr));

			rPacket->payload_len = ntohs(rPacket->ip.tot_len) - (rPacket->ip.ihl<<2) - (rPacket->tcp.doff * 4);
            if(rPacket->payload_len <= PAYLOAD_MAX_LEN)
                strncpy((char *)&rPacket->payload, (char *)(buffer + iphdrlen  + sizeof(struct ethhdr) + rPacket->tcp.doff * 4), rPacket->payload_len);
            else
                strcpy((char *)&rPacket->payload, "error(-1)");

			break;
		case 17:
			//UDP
            memcpy(&rPacket->udp, (buffer + iphdrlen + sizeof(struct ethhdr)), sizeof(struct udphdr));

			rPacket->payload_len = buflen - (iphdrlen + sizeof(struct ethhdr) + sizeof(struct udphdr));
            if(rPacket->payload_len <= PAYLOAD_MAX_LEN)
                strncpy((char *)&rPacket->payload, (char *)(buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr)), rPacket->payload_len);
            else
                strcpy((char *)&rPacket->payload, "error(-1)");

			break;
		default:
			//other
			rPacket -> other = 1;
			rPacket->payload_len = buflen - (iphdrlen + sizeof(struct ethhdr));
            strncpy((char *)&rPacket->payload, (char *)(buffer + iphdrlen  + sizeof(struct ethhdr)), rPacket->payload_len);
			break;
	}

	rPacket -> pPacket = buffer;

	return 1;
	//TODO: make remaining data
}