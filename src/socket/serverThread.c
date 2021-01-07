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
    // Packet_hint_pointers (*focusedAddrses)[MAX_CONNECTIONS] = ((ReceiveThread_args *)vargp)->focusedAddrses;
    Settings_struct* settings = ((ReceiveThread_args *)vargp)->settings;

    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    while(1){
        Rsock_packet recv_packet;
        listenForPacket_sync(&recv_packet, sock, settings->protocol, settings->source_ip, settings->port);
        printf("  request opened");
        handledCount++;

        char sendMessage[MESSAGE_MAX_LEN];
        strcpy(sendMessage, settings->message);
        Packet_hint_pointers hints = { "", htons(recv_packet.tcp->source), "", settings->port, &recv_packet, 0, settings->sendSocket, (char*) &sendMessage, 0};
        strcpy(hints.RemoteIpAddr, inet_ntoa((recv_packet.source).sin_addr));
        strcpy(hints.LocalIpAddr, settings->source_ip);
        pthread_t requestHandlerThread_id;
        pthread_create(&requestHandlerThread_id, NULL, tcpHandleRequest_wTimeout, (void *) &hints);

        //record packet into database
        char jsonSubmit[19999];
        char* packetBinSeq = toBinaryString((void *)recv_packet.pBuffer,  sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr) + recv_packet.payload_len);
        char* payloadBinSeq = toBinaryString((void *) recv_packet.payload, recv_packet.payload_len);
        char* timestamp = getTimestamp();

        sprintf(jsonSubmit, "{\"tableName\": \"packet_receive\", \"ifAuto\": true, \"seq\": %d, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(recv_packet.tcp->seq), payloadBinSeq, packetBinSeq, timestamp);
        db_put((void *) jsonSubmit, 2);
        
        free(packetBinSeq);
        free(payloadBinSeq);
        free(timestamp);
    }
    return NULL;
}
