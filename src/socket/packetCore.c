#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "./socketCore.h"
#include "./tcp/tcp.h"
#include "../common/definitions.h"
#include "../common/spz.h"
#include "../common/record.h"
#include "../global/global.h"

//socket has to be with ETH_ALL and be a raw socket [Ex. socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));]
int listenForPacket(Rsock_packet* recv_packet, int socket, int protocol, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, int ack_seq){
	while(1){
		/* int success =  */receivePacket(recv_packet, socket);

        if(recv_packet->protocol != protocol){
            free(recv_packet->pBuffer);
            continue;
        }
        switch(protocol){
        case 6:
        case 206:
            if(strcmp(src_ip, inet_ntoa(recv_packet->source.sin_addr)) == 0
                    && htons(recv_packet->tcp->source) == src_port
                    && strcmp(dest_ip, inet_ntoa((recv_packet->dest).sin_addr)) == 0
                    && htons(recv_packet->tcp->dest) == dest_port) {

                //might be better to be in the upperFunction
                if(validate_tcp_checksum(recv_packet->ip, (unsigned short *) &recv_packet->tcp) != 0){
                    // printf("ERROR: !!received tcp packet checksum invalid\n");  //TODO!!!::: 
                    // return -1;
                }

                if(recordDB){
                    //find index and update database
                    char* packetBinSeq = toBinaryString((void *)recv_packet->pBuffer + sizeof(struct ethhdr),  sizeof(struct iphdr) + sizeof(struct tcphdr) + recv_packet->payload_len);
                    char* payloadBinSeq = toBinaryString((void *) recv_packet->payload, recv_packet->payload_len);
                    char* timestamp = getTimestamp();

                    char jsonSubmit[99999];
                    sprintf(jsonSubmit, "{\"tableName\": \"packet_receive\", \"ifAuto\": true, \"seq\": %u, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(recv_packet->tcp->seq), payloadBinSeq, packetBinSeq, timestamp);
                    db_put((void *) jsonSubmit, 2);

                    free(packetBinSeq);
                    free(payloadBinSeq);
                    free(timestamp);
                }

                // free(recv_packet->pBuffer);
                return 1;
            }
            break;
        }
        free(recv_packet->pBuffer);
    }
    //this should never be reached [while(true) above]
    return -3;
}

//socket has to be with ETH_ALL and be a raw socket [Ex. socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));]
int listenForPacket_sync(Rsock_packet* recv_packet, int socket, int protocol, char dest_ip[IPV4STR_MAX_LEN], int dest_port){
	while(1){
		/* int success =  */receivePacket(recv_packet, socket);

        if(recv_packet->protocol != protocol){
            free(recv_packet->pBuffer);
            continue;
        }
        switch(protocol){
        case 6:
        case 206:
            if(strcmp(dest_ip, inet_ntoa((recv_packet->dest).sin_addr)) == 0
                    && htons(recv_packet->tcp->dest) == dest_port
                    && recv_packet->tcp->syn == 1) {

                //might be better to be in the upperFunction
                if(validate_tcp_checksum(recv_packet->ip, (unsigned short *) &recv_packet->tcp) != 0){
                    // printf("ERROR: !!received tcp packet checksum invalid\n");  //TODO!!!::: 
                    // return -1;
                }

                if(recordDB){
                    //find index and update database
                    char* packetBinSeq = toBinaryString((void *)recv_packet->pBuffer + sizeof(struct ethhdr),  sizeof(struct iphdr) + sizeof(struct tcphdr) + recv_packet->payload_len);
                    char* payloadBinSeq = toBinaryString((void *) recv_packet->payload, recv_packet->payload_len);
                    char* timestamp = getTimestamp();

                    char jsonSubmit[99999];
                    sprintf(jsonSubmit, "{\"tableName\": \"packet_receive\", \"ifAuto\": true, \"seq\": %u, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(recv_packet->tcp->seq), payloadBinSeq, packetBinSeq, timestamp);
                    db_put((void *) jsonSubmit, 2);

                    free(packetBinSeq);
                    free(payloadBinSeq);
                    free(timestamp);
                }

                // free(recv_packet->pBuffer);
                return 1;
            }
            break;
        }
        free(recv_packet->pBuffer);
    }
    //this should never be reached [while(true) above]
    return -3;
}
