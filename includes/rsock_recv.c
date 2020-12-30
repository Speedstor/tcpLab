
//include libraries, straight copying for now
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include<netinet/tcp.h>

#include <linux/if_packet.h>
#include <arpa/inet.h>


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

struct rsock_packet* recievePacket(int sock_r){
	//reception of the network packet || recieve the network packet
	unsigned char *buffer = (unsigned char *) malloc(65536); //max buffer memory size
	memset(buffer, 0, 65536);
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);


	//reveive a network packet and copy in to buffer
	int buflen = recvfrom(sock_r, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);
	if(buflen < 0) {
		printf("error in reading recvfrom function \n");
		return NULL;
	}

	//extract ethernet header
	struct ethhdr *eth = (struct ethhdr *)(buffer);

	//extract ip header
	struct sockaddr_in source, dest;
	struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = ip->saddr;
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = ip->daddr;
	

	//extract transport layer header
	int iphdrlen; // getting actual size of IP header
	iphdrlen = ip -> ihl * 4;
	struct tcphdr *tcp;
	struct udphdr *udp;

	unsigned char * data;
	int remaining_data_len;
	struct rsock_packet* returnVar;
	returnVar = (struct rsock_packet*) malloc(sizeof(struct rsock_packet));

	returnVar -> eth = *eth;
	returnVar -> ip = *ip;
	returnVar -> source = source;
	returnVar -> dest = dest;
	returnVar -> protocol = ip->protocol;

	switch(ip->protocol){
		case 6:
			//tcp
			tcp = (struct tcphdr*)( buffer + iphdrlen + sizeof(struct ethhdr));

			data = (unsigned char *) (buffer + iphdrlen  + sizeof(struct ethhdr) + tcp->doff * 4);
			remaining_data_len = ntohs(ip->tot_len) - (ip->ihl<<2) - (tcp->doff * 4);
			// remaining_data_len = buflen - (iphdrlen + sizeof(struct ethhdr) + tcp->doff * 4);

			returnVar -> tcp = tcp;
			returnVar -> payload_len = remaining_data_len;
			returnVar -> payload = data;
			break;
		case 17:
			//UDP
			// getting pointer to udp header
			udp = (struct udphdr*)( buffer + iphdrlen + sizeof(struct ethhdr));

			data = (buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));
			remaining_data_len = buflen - (iphdrlen + sizeof(struct ethhdr) + sizeof(struct udphdr));

			returnVar -> udp = udp;
			returnVar -> payload_len = remaining_data_len;
			returnVar -> payload = data;
			break;
		default:
			//other
			data = (buffer + iphdrlen  + sizeof(struct ethhdr));
			remaining_data_len = buflen - (iphdrlen + sizeof(struct ethhdr));
			
			returnVar -> other = 1;
			returnVar -> payload_len = remaining_data_len;
			returnVar -> payload = data;
			break;
	}

	returnVar -> pPacket = buffer;

	// printf("\n");
	return returnVar;
	//TODO: make remaining data

}

char* FileToString(char* path){
	FILE* buffer_txt;
	buffer_txt = fopen(path, "rb");
	char *returnString = 0;
	if(buffer_txt){
		fseek(buffer_txt, 0, SEEK_END);
		long length = ftell(buffer_txt);
  		fseek (buffer_txt, 0, SEEK_SET);
		returnString = malloc(length);
		if(returnString) {
			fread(returnString, 1, length, buffer_txt);
		}
		fclose(buffer_txt);
		return returnString;
	}
	return NULL;
}

char* DebugEthernetHeader(struct ethhdr *eth){
	//str_builder_t *sb;
	//sb = str_builder_create();

	FILE* buffer_txt;
	buffer_txt = fopen("buffer.txt", "w");
	if(!buffer_txt) {
		printf("unable to open buffer file:: error");
		return "\0";
	}

	fprintf(buffer_txt, "\nEthernet Header\n");
	fprintf(buffer_txt, "\t|-Source Address\t: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
	fprintf(buffer_txt, "\t|-Destination Address\t: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
	fprintf(buffer_txt, "\t|-Protocol\t: %d\n",eth->h_proto);
	fclose(buffer_txt);

	//read it back and put it in char array (slower, but only for debug i guess?, i guess not, but this is for now);
	return FileToString("buffer.txt");
}

char* DebugIPHeader(struct iphdr *ip, struct sockaddr_in source, struct sockaddr_in dest){
	FILE* buffer_txt;
	buffer_txt = fopen("buffer.txt", "w");
	if(!buffer_txt) {
		printf("unable to open buffer file:: error");
		return "\0";
	}

	fprintf(buffer_txt, "IP Header\n");
	fprintf(buffer_txt, "\t|-Internet Header Length\t: %d DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
	fprintf(buffer_txt, "\t|-Type Of Service\t: %d\n",(unsigned int)ip->tos);
	fprintf(buffer_txt, "\t|-Total Length\t: %d Bytes\n",ntohs(ip->tot_len));
	fprintf(buffer_txt, "\t|-Identification\t: %d\n",ntohs(ip->id));
	fprintf(buffer_txt, "\t|-Time To Live\t: %d\n",(unsigned int)ip->ttl);
	fprintf(buffer_txt, "\t|-Protocol\t: %d\n",(unsigned int)ip->protocol);
	fprintf(buffer_txt, "\t|-Header Checksum\t: %d\n",ntohs(ip->check));
	fprintf(buffer_txt, "\t|-Source IP\t: %s\n", inet_ntoa(source.sin_addr));
	fprintf(buffer_txt, "\t|-Destination IP\t: %s\n",inet_ntoa(dest.sin_addr));
	fclose(buffer_txt);

	//read it back and put it in char array (slower, but only for debug i guess?, i guess not, but this is for now);
	return FileToString("buffer.txt");
}

char* DebugUdpHeader(struct udphdr *udp){
	FILE* buffer_txt;
	buffer_txt = fopen("buffer.txt", "w");
	if(!buffer_txt) {
		printf("unable to open buffer file:: error");
		return "\0";
	}
	
	fprintf(buffer_txt, "UDP Header\n");
	fprintf(buffer_txt, "\t|-SourcePort\t: %d\n", ntohs(udp -> source));
	fprintf(buffer_txt, "\t|-Destination Port: %d\n", ntohs(udp -> dest));
	fprintf(buffer_txt, "\t|=UDP Length\t: %d\n", ntohs(udp -> len));
	fprintf(buffer_txt, "\t|-UDP Checksum\t: %d\n", ntohs(udp -> check));
	fclose(buffer_txt);

	
	//read it back and put it in char array (slower, but only for debug i guess?, i guess not, but this is for now);
	return FileToString("buffer.txt");
}

char* DebugTcpHeader(struct tcphdr *tcp){
	FILE* buffer_txt;
	buffer_txt = fopen("buffer.txt", "w");
	if(!buffer_txt) {
		printf("unable to open buffer file:: error");
		return "\0";
	}
	
	fprintf(buffer_txt, "TCP Header\n");
	fprintf(buffer_txt, "\t|-SourcePort\t: %u\n", ntohs(tcp -> source));
	fprintf(buffer_txt, "\t|-Destination Port: %u\n", ntohs(tcp -> dest));
	fprintf(buffer_txt, "\t|-Sequence Number\t: %u\n", 	ntohl(tcp -> seq));
	fprintf(buffer_txt, "\t|-Acknowledge Seq\t: %u\n", ntohl(tcp -> ack_seq));
	fprintf(buffer_txt, "\t|-Header Length\t: %d DWORDS or %d BYTES\n" ,(unsigned int)tcp->doff,(unsigned int)tcp->doff*4); //maybe break
	fprintf(buffer_txt, "\t|--------Flags----------\n");
	fprintf(buffer_txt, "\t\t|-Urgent Flag\t: %d\n", (unsigned int) tcp -> urg);
	fprintf(buffer_txt, "\t\t|-Ack Flag\t: %d\n", (unsigned int) tcp -> ack);
	fprintf(buffer_txt, "\t\t|-Push Flag\t: %d\n", (unsigned int) tcp -> psh);
	fprintf(buffer_txt, "\t\t|-Reset Flag\t: %d\n", (unsigned int) tcp -> rst);
	fprintf(buffer_txt, "\t\t|-Synchronise Flag\t: %d\n", (unsigned int) tcp -> syn);
	fprintf(buffer_txt, "\t\t|-Finish Flag\t: %d\n", (unsigned int) tcp -> fin);
	fprintf(buffer_txt, "\t|-Window size\t: %d\n", ntohs(tcp -> window));
	fprintf(buffer_txt, "\t|-Checksum\t: %d\n", ntohs(tcp -> check));
	fprintf(buffer_txt, "\t|-Urgent Pointer\t: %d\n", ntohs(tcp -> urg_ptr));
	fclose(buffer_txt);

	
	//read it back and put it in char array (slower, but only for debug i guess?, i guess not, but this is for now);
	return FileToString("buffer.txt");
}