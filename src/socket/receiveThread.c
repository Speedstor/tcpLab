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
#include "./socketCore.h"


void* receiveThread(void* vargp){
    Packet_hint_pointers (*focusedAddrses)[MAX_CONNECTIONS] = ((ReceiveThread_args *)vargp)->focusedAddrses;
    Settings_struct* settings = ((ReceiveThread_args *)vargp)->settings;

    int socket = settings->receiveSocket;

	while(1){
        Rsock_packet receive;
    	memset(&receive, 0, sizeof(Rsock_packet));

		int rSuccess = receivePacket(&receive, socket);
        if(rSuccess < 0) continue; //bad packet and error in receiving

        if(receive.protocol != 6 && receive.protocol != 206) continue; //only tcp packets are supported for now
        for(int i=0; i < MAX_CONNECTIONS; i++){
            if((*focusedAddrses)[i].flag != 0 && (*focusedAddrses)[i].flag != 404){
                if(strcmp((*focusedAddrses)[i].RemoteIpAddr, inet_ntoa(receive.source.sin_addr)) == 0
                  && htons(receive.tcp.dest) == (*focusedAddrses)[i].local_port
                  && htons(receive.tcp.source) == (*focusedAddrses)[i].remote_port
                  && strcmp((*focusedAddrses)[i].LocalIpAddr, inet_ntoa((receive.dest).sin_addr)) == 0) {

                      
                }
            }
        }
        free(receive.pPacket);
    }
    return NULL;
}