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
#include "../global/global.h"

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
    if(verbose != 1) printf("requesting...  ");
    fflush(stdout);
    handledCount++;
    struct sockaddr_in serv_addr;
    char finalMsg[MESSAGE_MAX_LEN];
    memset(&finalMsg, '\0', MESSAGE_MAX_LEN);


    //check if port open, or get available free port if port is 0
    if(checksumType == 1){

        int ifavl = -1;
        while(ifavl < 0){
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(49152 + (rand() % 16300));
            inet_aton(source_ip, &serv_addr.sin_addr);

            if(bind(socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)  ifavl = 1;
        }
    }else{
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(49152 + (rand() % 16300));
        inet_aton(source_ip, &serv_addr.sin_addr);

    }

    switch(protocol){
        case 6:                                        //standard tcp
        case 206:                                      //custom tcp
        case 216:                                      //custom tcp
// int tcp_request_singleThread(int send_socket, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, char requestMsg[PAYLOAD_MAX_LEN], char* finalMsg){
            tcp_request_wTimeout(3, socket, dest_ip, dest_port, source_ip, ntohs(serv_addr.sin_port), requestMsg, (char *) &finalMsg);
            break;
            break;
        case 7:                                        //standard udp
            break;
        case 207:                                      //custom udp
            return -1;
            break;
    }

    if(strlen(finalMsg) > 0) printf("Received:  %s\n", finalMsg);
    return 1;
}

int receivePacket(Rsock_packet* rPacket, int sock_r){
	//reception of the network packet || receive the network packet
	rPacket->pBuffer = (unsigned char *) malloc(65536); //max buffer memory size
	memset(rPacket->pBuffer, 0, 65536);

	//receive a network packet and copy in to buffer
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	int buflen = recvfrom(sock_r, rPacket->pBuffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);
	if(buflen < 0) {
        free(rPacket->pBuffer);
		return -1;
	}

	//point to ethernet header                                   ETHERNET
    rPacket->eth = (struct ethhdr *)(rPacket->pBuffer);

	//point to ip header & get ips of source and destination     IP
	rPacket->ip = (struct iphdr*)(rPacket->pBuffer + sizeof(struct ethhdr));
	rPacket->source.sin_addr.s_addr = rPacket->ip->saddr;
	rPacket->dest.sin_addr.s_addr = rPacket->ip->daddr;
	rPacket->protocol = rPacket->ip->protocol;

	//extract transport layer header                            TRANSPORT
	int iphdrlen = rPacket->ip->ihl * 4;// getting size of IP header from header (more accurate/foolproof)
	switch(rPacket->protocol){
		case 216:
		case 206:
		case 6:
			//tcp
			rPacket->tcp = (struct tcphdr*)( rPacket->pBuffer + iphdrlen + sizeof(struct ethhdr));

			rPacket->payload_len = ntohs(rPacket->ip->tot_len) - (rPacket->ip->ihl<<2) - (rPacket->tcp->doff * 4);
			rPacket->payload = (unsigned char *) (rPacket->pBuffer + iphdrlen  + sizeof(struct ethhdr) + rPacket->tcp->doff * 4);
            // memset((rPacket->payload + rPacket->payload_len) , '\0', 1);

			break;
		case 17:
			//UDP
			rPacket->udp = (struct udphdr*)( rPacket->pBuffer + iphdrlen + sizeof(struct ethhdr));
            
			rPacket->payload_len = buflen - (iphdrlen + sizeof(struct ethhdr) + sizeof(struct udphdr));
			rPacket->payload = (unsigned char *) (rPacket->pBuffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));
            // memset((rPacket->payload + rPacket->payload_len) , '\0', 1);

			break;
		default:
			//other
			rPacket -> other = 1;
			rPacket->payload_len = buflen - (iphdrlen + sizeof(struct ethhdr));
			rPacket->payload = (unsigned char *)(rPacket->pBuffer + iphdrlen  + sizeof(struct ethhdr));
			break;
	}

	return 1;
}


//for "multithread", combine all the listenForPacket() into one socket
void* receiveThread(void* vargp){
    Packet_hint_pointers (*focusedAddrses)[SERVER_MAX_CONNECTIONS] = ((ReceiveThread_args *)vargp)->focusedAddrses;
    Settings_struct* settings = ((ReceiveThread_args *)vargp)->settings;

    int socket = settings->receiveSocket;

	while(running){
        Rsock_packet receive;
    	memset(&receive, 0, sizeof(Rsock_packet));

		int rSuccess = receivePacket(&receive, socket);
        if(rSuccess < 0) continue; //bad packet and error in receiving

        if(receive.protocol != 6 && receive.protocol != 206) continue; //only tcp packets are supported for now
        for(int i=0; i < SERVER_MAX_CONNECTIONS; i++){
            if((*focusedAddrses)[i].flag != 0 && (*focusedAddrses)[i].flag != 404){
                if(strcmp((*focusedAddrses)[i].RemoteIpAddr, inet_ntoa(receive.source.sin_addr)) == 0
                  && htons(receive.tcp->dest) == (*focusedAddrses)[i].local_port
                  && htons(receive.tcp->source) == (*focusedAddrses)[i].remote_port
                  && strcmp((*focusedAddrses)[i].LocalIpAddr, inet_ntoa((receive.dest).sin_addr)) == 0) {
                    
                      
                }
            }
        }
        free(receive.pBuffer);
    }
    return NULL;
}