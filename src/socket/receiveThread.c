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
#include "../common/record.h"
#include "../common/spz.h"
#include "./tcp/tcp.h"
#include "./socketCore.h"
#include "../global/global.h"


void* serverThread(void* vargp){
    // Packet_hint_pointers (*focusedAddrses)[MAX_CONNECTIONS] = ((ReceiveThread_args *)vargp)->focusedAddrses;
    Settings_struct* settings = ((ReceiveThread_args *)vargp)->settings;

    int socket = settings->receiveSocket;

    switch(settings->protocol){
    case 6:
        while(running){
            Rsock_packet receive;
            memset(&receive, 0, sizeof(Rsock_packet));

            int rSuccess = receivePacket(&receive, socket);
            // if(rSuccess < 0) continue; //bad packet and error in receiving

            // if(receive.protocol != settings->protocol) continue; //only tcp packets are supported for now

            if(strcmp(settings->source_ip, inet_ntoa((receive.dest).sin_addr)) == 0 && htons(receive.tcp.dest) == settings->port && receive.tcp.syn == 1) {
                printf("received\n");
                fflush(stdout);
                
                //record it to the packets to be looking out for
                char sendMessage[MESSAGE_MAX_LEN];
                strcpy(sendMessage, settings->message);
                Packet_hint_pointers hints = { "", htons(receive.tcp.source), "", settings->port, &receive, 0, settings->sendSocket, (char*) &sendMessage, 0};
                strcpy(hints.RemoteIpAddr, inet_ntoa((receive.source).sin_addr));
                strcpy(hints.LocalIpAddr, settings->source_ip);
                pthread_t requestHandlerThread_id;
                pthread_create(&requestHandlerThread_id, NULL, tcpHandleRequest_singleThread, (void *) &hints);
            
            
                //record packet into database
                char jsonSubmit[19999];
                char* packetBinSeq = toBinaryString((void *)receive.pPacket,  sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr) + receive.payload_len);
                char* payloadBinSeq = toBinaryString((void *) receive.payload, receive.payload_len);
                char* timestamp = getTimestamp();

                sprintf(jsonSubmit, "{\"tableName\": \"packet_receive\", \"ifAuto\": true, \"seq\": %d, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(receive.tcp.seq), payloadBinSeq, packetBinSeq, timestamp);
                db_put((void *) jsonSubmit, 2);
                
                free(packetBinSeq);
                free(payloadBinSeq);
                free(timestamp);
            }else{
                free(receive.pPacket);
            }
        }
        break;
    }
    return NULL;
}