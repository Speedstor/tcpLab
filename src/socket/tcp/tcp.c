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

#include "../../common/definitions.h"


int tcp_request(int socket, int destPort, struct sockaddr_in serv_addr, char dest_ip[IPV4STR_MAX_LEN], char requestMsg[PAYLOAD_MAX_LEN], char* finalMsg, Packet_hint_pointers (* focusedAddrses)[MAX_CONNECTIONS]){
    char debugText[150];
    Rsock_packet recv_packet;

    sprintf(debugText, "port number %d || addr: %d\n", ntohs(serv_addr.sin_port), serv_addr.sin_addr.s_addr);
    // print_progressBar(debugText, 10);

    int i;
    for(i=0; i < MAX_CONNECTIONS; i++){
        if(focusedAddrses[i]->flag == 0 || focusedAddrses[i]->flag == 404) {
            struct Packet_hint_pointers buffer = {
                {*dest_ip},
                destPort,
                {*inet_ntoa(serv_addr.sin_addr)},
                ntohs(serv_addr.sin_port),
                &recv_packet,
                1,
                socket,
                finalMsg
            };
            (*focusedAddrses)[i] = buffer;
            if(1){
                sprintf(debugText, "focusedAddrses[%d]: %s | %d | %s | %d\n", i, (char *) focusedAddrses[i]->LocalIpAddr, focusedAddrses[i]->local_port, (char*) focusedAddrses[i]->pTargetSet, focusedAddrses[i]->flag);
                // print_progressBar(debugText, 13);
            }
            break;
        }else{
            if(i == MAX_CONNECTIONS) {
                // print_progressBar("connection full (%d), please close some connection or wait and try again", 100);
                return -2;
            }
        }
    }
    return 1;
}
