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

#include "../src/common/definitions.h"

int snippetsOfTcpReceiveManage(){
    //to avoid errors, i hate the color red
    Rsock_packet* recv_packet;
    Packet_hint_pointers focusedAddrses[MAX_CONNECTIONS];
    
    /* source ip variable in string */
    char srcIp[IPV4STR_MAX_LEN];
    strncpy(srcIp, focusedAddrses[i].pSrcIpAddr, IPV4STR_MAX_LEN);
        
        //acknowledge packets, sync with the client
        if(recv_packet->payload_len > 0){
            int msgLen = recv_packet->payload_len;
            if(recv_packet->tcp.syn == 1) msgLen++;

            uint32_t seq = ntohl(recv_packet->tcp.ack_seq);
            uint32_t ack_seq = ntohl(recv_packet->tcp.seq) + msgLen; //add one for the syn flag

            char tcpOptions[50];
            memset(&tcpOptions, '\0', 50);
            sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
            send_packet(focusedAddrses[i].sock, 6, focusedAddrses[i].dest_port, htons(recv_packet->tcp.source), srcIp, "", tcpOptions);
        }

        //ending packet -> finish tags
        if(recv_packet->tcp.fin == 1){
            if(/* send_mode ==  */1){
                int msgLen = recv_packet->payload_len;
                if(recv_packet->tcp.syn == 1) msgLen++;

                uint32_t seq = ntohl(recv_packet->tcp.ack_seq);
                uint32_t ack_seq = ntohl(recv_packet->tcp.seq) + msgLen + 1; //add one for the syn flag

                char tcpOptions[50];
                memset(&tcpOptions, '\0', 50);
                sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
                send_packet(focusedAddrses[i].sock, 6, focusedAddrses[i].dest_port, htons(recv_packet->tcp.source), srcIp, "", tcpOptions);

                memset(&tcpOptions, '\0', 50);
                sprintf(tcpOptions, "f\t%u\t%u\t", seq, ack_seq);
                send_packet(focusedAddrses[i].sock, 6, focusedAddrses[i].dest_port, htons(recv_packet->tcp.source), srcIp, "", tcpOptions);
            }

            focusedAddrses[i].flag = 404; //404 waiting for acknowledge fin bit
        }

        //copy payload if there is any
        if(recv_packet->payload_len > 0 && focusedAddrses[i].msg != NULL) {
            strncpy(focusedAddrses[i].msg + strlen(focusedAddrses[i].msg), (char *) recv_packet->payload, recv_packet->payload_len);
        }

        //packets that are not purely acknowledge, save to the focusAddrses to save
        if((recv_packet->payload_len > 0 || recv_packet->tcp.psh == 1 || recv_packet->tcp.syn == 1 || recv_packet->tcp.fin == 1 || recv_packet->tcp.rst == 1) && !(recv_packet->tcp.syn == 1 && recv_packet->tcp.ack == 0)){
            //set to not free the whole variable || whatever that means
            // savePacket = 1;

            //free the previous packet
            if(focusedAddrses[i].flag != 1 && focusedAddrses[i].flag != 0 && focusedAddrses[i].flag != 404){
                free(focusedAddrses[i].pTargetSet->pBuffer);
            }

            // struct rsock_packet recieveCopy = *recv_packet->
            // *focusedAddrses[i].pTargetSet = recieveCopy;
            if(focusedAddrses[i].flag == 1){
                focusedAddrses[i].flag = 2;
            }else if(focusedAddrses[i].flag == 404){
                
            }else{
                focusedAddrses[i].flag = 3;
            }
        }
}

int tcp_request_multiThread(int socket, int dest_port, struct sockaddr_in serv_addr, char dest_ip[IPV4STR_MAX_LEN], char requestMsg[PAYLOAD_MAX_LEN], char* finalMsg, Packet_hint_pointers (* focusedAddrses)[MAX_CONNECTIONS]){
    progressBar_print("started tcp request", 1);
    char debugText[150];
    Rsock_packet recv_packet;
    char src_ip[IPV4STR_MAX_LEN] = {*inet_ntoa(serv_addr.sin_addr)};
    int src_port = ntohs(serv_addr.sin_port);

    sprintf(debugText, "port number %d || addr: %d\n", ntohs(serv_addr.sin_port), serv_addr.sin_addr.s_addr);
    progressBar_print(debugText, 10);
    
    struct addrinfo *dest, *source;
    int error = getaddrinfo(dest_ip, NULL, &addr_settings, &dest);
    error = getaddrinfo(source_ip, NULL, &addr_settings, &source);
    if (error) {
        printf("Destination ip invalid: %s\n", dest_ip);
        return -1;
    }
    
    int i;
    for(i=0; i < MAX_CONNECTIONS; i++){
        if(focusedAddrses[i]->flag == 0 || focusedAddrses[i]->flag == 404) {
            struct Packet_hint_pointers buffer = {
                {*dest_ip},                         // remoteIpAddr
                dest_port,                           // remote_port
                src_ip,                             // LocalIpAddr
                src_port,                           // local_port 
                &recv_packet,                       // pTargetSet
                1,                                  // flag
                socket,                             // sock
                finalMsg,                           // msg
                -1                                  // is writing
            };
            (*focusedAddrses)[i] = buffer;

            sprintf(debugText, "focusedAddrses[%d]: %s | %d | %s | %d\n", i, (char *) focusedAddrses[i]->LocalIpAddr, focusedAddrses[i]->local_port, (char*) focusedAddrses[i]->pTargetSet, focusedAddrses[i]->flag);
            progressBar_print(debugText, 13);
            break;
        }else{
            if(i == MAX_CONNECTIONS) {
                progressBar_print("connection full (%d), please close some connection or wait and try again", 100);
                return -2;
            }
        }
    }


    //initialize random sequence number
    uint32_t seq = rand() % 1000000000;
    uint32_t ack_seq = 0;
    char tcpOptions[50];

    /** 
     * First Packet  ------
     */
    progressBar_print("initiating tcp handshake................", 20);
    
    sprintf(tcpOptions, "s\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(socket, src_port, dest_port, ntohs(serv_addr.sin_port), destPort, dest_ip, "", tcpOptions);

    /** 
     * Receive Packet  ------
     */        
    progressBar_print("waiting response................", 25);

    while(focusedAddrses[i]->flag == 1) msleep(100); //wait for packet

    if(recv_packet.tcp.syn == 1 && recv_packet.tcp.ack == 1){    
        progressBar_print("received!! -- finished 3-way handshake -> sending actual request", 30);
    }else if(recv_packet.tcp.rst == 1){
        progressBar_print("acknowledge skipped || lost packet || error in ip", 100);
        focusedAddrses[i]->flag = 0;
        return -1;
    }else{
        progressBar_print("unkown flag condition!!", 100);
        focusedAddrses[i]->flag = 0;
        return -1;
    }

    /** 
     * Third Packet  ------
     */
    seq = ntohl(recv_packet.tcp.ack_seq);
    ack_seq = ntohl(recv_packet.tcp.seq) + 1; //add one for the syn flag

    sprintf(tcpOptions, "pa\t%d\t%d\t", seq, ack_seq);

    send_packet(socket, protocol, ntohs(serv_addr.sin_port), port, dest_ip, message, tcpOptions);

    /** 
     * Receive Packet  ------
     */
    progressBar_print("waiting for whole data and fin bit...", 70);

    while(focusedAddrses[i]->flag != 404) {
        sleep(1);
    }
    
    end_progressBar(0);
    printf("%s\n", focusedAddrses[i]->msg);

    focusedAddrses[i]->flag = 0;
    free(finalMsg);
        
    return 1;
}



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
            if(rSuccess < 0) continue; //bad packet and error in receiving

            // if(receive.protocol != settings->protocol) continue; //only tcp packets are supported for now

            if(strcmp(settings->source_ip, inet_ntoa((receive.dest).sin_addr)) == 0 && htons(receive.tcp->dest) == settings->port && receive.tcp->syn == 1) {
                printf("received\n");
                fflush(stdout);
                
                //record it to the packets to be looking out for
                char sendMessage[MESSAGE_MAX_LEN];
                strcpy(sendMessage, settings->message);
                Packet_hint_pointers hints = { "", htons(receive.tcp->source), "", settings->port, &receive, 0, settings->sendSocket, (char*) &sendMessage, 0};
                strcpy(hints.RemoteIpAddr, inet_ntoa((receive.source).sin_addr));
                strcpy(hints.LocalIpAddr, settings->source_ip);
                pthread_t requestHandlerThread_id;
                pthread_create(&requestHandlerThread_id, NULL, tcpHandleRequest_singleThread, (void *) &hints);
            
            
                //record packet into database
                char jsonSubmit[19999];
                char* packetBinSeq = toBinaryString((void *)receive.pBuffer,  sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr) + receive.payload_len);
                char* payloadBinSeq = toBinaryString((void *) receive.payload, receive.payload_len);
                char* timestamp = getTimestamp();

                sprintf(jsonSubmit, "{\"tableName\": \"packet_receive\", \"ifAuto\": true, \"seq\": %d, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(receive.tcp->seq), payloadBinSeq, packetBinSeq, timestamp);
                db_put((void *) jsonSubmit, 2);
                
                free(packetBinSeq);
                free(payloadBinSeq);
                free(timestamp);
            }else{
                free(receive.pBuffer);
            }
        }
        break;
    }
    return NULL;
}