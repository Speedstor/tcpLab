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
#include "./packetCore.h"

void* serverThread(void* vargp){
    // Packet_hint_pointers (*focusedAddrses)[SERVER_MAX_CONNECTIONS] = ((ReceiveThread_args *)vargp)->focusedAddrses;
    Settings_struct* settings = ((ReceiveThread_args *)vargp)->settings;

    pthread_t requestHandlerThread_ids[SERVER_MAX_CONNECTIONS];
    volatile int threadComplete[SERVER_MAX_CONNECTIONS] = {0};
    int threadIndex = 0;

    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    while(1){
        Rsock_packet recv_packet;
        listenForPacket_sync(&recv_packet, sock, settings->protocol, settings->source_ip, settings->port);
        handledCount++;

        char sendMessage[MESSAGE_MAX_LEN];
        strcpy(sendMessage, settings->message);
        Packet_hint_pointers hints = { "", htons(recv_packet.tcp->source), "", settings->port, &recv_packet, ntohl(recv_packet.tcp->seq), settings->sendSocket, (char*) &sendMessage, 0};
        strcpy(hints.RemoteIpAddr, inet_ntoa((recv_packet.source).sin_addr));
        strcpy(hints.LocalIpAddr, settings->source_ip);

        //find thread for the request
        threadIndex++;
        if(threadIndex >= SERVER_MAX_CONNECTIONS) threadIndex = 0;
        int count = 0;
        if(threadComplete[threadIndex] == 2){
            pthread_join(requestHandlerThread_ids[threadIndex], NULL);
            threadComplete[threadIndex] = 0;
        }
        while(threadComplete[threadIndex] != 0) {
            if(threadComplete[threadIndex] == 2){
                pthread_join(requestHandlerThread_ids[threadIndex], NULL);
                threadComplete[threadIndex] = 0;
                break;
            }
            threadIndex++;
            count++;
            if(threadIndex >= SERVER_MAX_CONNECTIONS) threadIndex = 0;
            if(count > SERVER_MAX_CONNECTIONS) sleep(1);
        }
        hints.expansion_ptr = (void*) &threadComplete[threadIndex];
        pthread_create(&requestHandlerThread_ids[threadIndex], NULL, tcpHandleRequest_wTimeout, (void *) &hints);
        threadComplete[threadIndex] = -1;
        
        free(recv_packet.pBuffer);
    }
    return NULL;
}
