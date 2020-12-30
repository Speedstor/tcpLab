#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>



#ifndef RSOCK_STRUCT
#define RSOCK_STRUCT

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

#ifndef RSOCK_RECV_H
#define RSOCK_RECV_H

struct rsock_packet* recievePacket(int sock_r);

#endif


#ifndef FILE_TO_STRING
#define FILE_TO_STRING

char* FileToString(char* path);

#endif

#ifndef RSOCK_DEBUG
#define RSOCK_DEBUG


char* DebugEthernetHeader(struct ethhdr *eth);
char* DebugIPHeader(struct iphdr *ip, struct sockaddr_in source, struct sockaddr_in dest);
char* DebugUdpHeader(struct udphdr *udp);
char* DebugTcpHeader(struct tcphdr *tcp);

#endif