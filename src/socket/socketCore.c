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

int receivePacket(Rsock_packet* rPacket, int sock_r);

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

void* receiveThread(void* vargp){
    struct packet_hint_pointers (*focusedAddrses)[MAX_CONNECTIONS] = ((ReceiveThread_args *)vargp)->focusedAddrses;
    Settings_struct* settings = ((ReceiveThread_args *)vargp)->settings;

    int socket = settings->receiveSocket;

	while(1){
        Rsock_packet receive; 
    	memset(&receive, 0, sizeof(Rsock_packet));

		int rSuccess = receivePacket(&receive, socket);
        if(rSuccess < 0) continue;

        if(receive.protocol != 6) continue; //only tcp packets are succored for now
        for(int i=0; i < MAX_CONNECTIONS; i++){
            if((*focusedAddrses)[i].flag != 0 && (*focusedAddrses)[i].flag != 404){
                if(strcmp((*focusedAddrses)[i].pSrcIpAddr, inet_ntoa(receive.source.sin_addr)) == 0 
                  && htons(receive.tcp.dest) == (*focusedAddrses)[i].dest_port 
                  && htons(receive.tcp.source) == (*focusedAddrses)[i].src_port
                  && strcmp((*focusedAddrses)[i].pDestIpAddr, inet_ntoa((receive.dest).sin_addr)) == 0) {
                      

                }
            }
        }
        free(receive.pPacket);
    }


    return NULL;
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
		// printf("error in reading recvfrom function in receivePacket() \n");
		return -1;
	}

	//extract ethernet header                                   ETHERNET
    memcpy(&rPacket->eth, buffer, sizeof(struct ethhdr));
	// struct ethhdr *eth = (struct ethhdr *)(buffer);


	//extract ip header & get ips of source and destination     IP
    memcpy(&rPacket->ip, (buffer + sizeof(struct ethhdr)), sizeof(struct iphdr));
	struct sockaddr_in source, dest;
	// memset(&source, 0, sizeof(source));                                  //WARNING:: not tested without yet
	// memset(&dest, 0, sizeof(dest));                                      //WARNING:: not tested without yet
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
            if(rPacket->payload_len <= PAYLOAD_MAX_LEN)  {
                strncpy((char *)&rPacket->payload, (char *)(buffer + iphdrlen  + sizeof(struct ethhdr) + rPacket->tcp.doff * 4), rPacket->payload_len);
            }
            else {
                // returnVar.payload = "error(-1)";
            }
			// remaining_data_len = buflen - (iphdrlen + sizeof(struct ethhdr) + tcp->doff * 4);

			break;
		case 17:
			//UDP
            memcpy(&rPacket->udp, (buffer + iphdrlen + sizeof(struct ethhdr)), sizeof(struct udphdr));

			rPacket->payload_len = buflen - (iphdrlen + sizeof(struct ethhdr) + sizeof(struct udphdr));
            if(rPacket->payload_len <= PAYLOAD_MAX_LEN)  {
                strncpy((char *)&rPacket->payload, (char *)(buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr)), rPacket->payload_len);
            }else
            {
                // returnVar.payload = "error(-1)";
            }
            
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