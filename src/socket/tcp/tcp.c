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
#include <sys/socket.h>


#include<sys/socket.h>
#include<sys/types.h>
#include<sys/ioctl.h>

#include<net/if.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/if_ether.h>
#include<netinet/udp.h>

#include<linux/if_packet.h>

#include "../../common/definitions.h"
#include "../../common/record.h"
#include "../../global/global.h"
#include "../packetCore.h"
#include "../../common/spz.h"

unsigned short validate_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
unsigned short compute_tcp_checksum_withFullPacket(void* fullPacket);
unsigned short compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
unsigned short csum(void *packetPtr,int nbytes) ;
unsigned short compute_tcpcrc_checksum(void* fullPacket);
int tcp_sendPacket(int sock, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions);

void* tcpHandleRequest_singleThread(void* vargp){
    int protocol = getProtocol(checksumType);
    //receive sync
    Packet_hint_pointers* hints;
    hints = vargp;
    char predefinedResponse[MESSAGE_MAX_LEN];
    strcpy(predefinedResponse, hints->msg);

    progressBar_print(1, "start handling req: %s", hints->RemoteIpAddr);

    struct addrinfo *dest, *source;
    int error = getaddrinfo(hints->RemoteIpAddr, NULL, &addr_settings, &dest);
    error = getaddrinfo(hints->LocalIpAddr, NULL, &addr_settings, &source);
    if (error) {
        printf("Destination ip invalid: %s\n", hints->RemoteIpAddr);
        return NULL;
    }

    progressBar_print(10, "initialized variables for client req");

    Rsock_packet recv_packet;
    char tcpOptions[50];

    uint32_t seq = rand() % 1000000000;
    uint32_t ack_seq = hints->flag + 1;

    int send_socket = hints->sock;
	int recv_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(send_socket < 0 || recv_socket < 0){
        progressBar_print(100, "(-1) open socket error in handle request thread");
    }

    progressBar_print(20, "send socket: %d", send_socket);

    //send sync ack
    sprintf(tcpOptions, "sa\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, hints->local_port, hints->remote_port, dest, source, "", tcpOptions);

    progressBar_print(30, "sent syn + ack packet");

    //last handshake packet: get ack and parse data (the upload request string)
    listenForPacket(&recv_packet, recv_socket, protocol, (char *) &hints->LocalIpAddr, hints->local_port, (char *) &hints->RemoteIpAddr, hints->remote_port, seq+=1);
    if(recv_packet.tcp->ack == 1) //normal behavior;
    free(recv_packet.pBuffer);

    //push ack packet
    listenForPacket(&recv_packet, recv_socket, protocol, (char *) &hints->LocalIpAddr, hints->local_port, (char *) &hints->RemoteIpAddr, hints->remote_port, seq);
    ack_seq = ntohl(recv_packet.tcp->seq) + recv_packet.payload_len;
    if(recv_packet.tcp->syn == 1) ack_seq++;
    sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
    tcp_sendPacket(send_socket, hints->local_port, hints->remote_port, dest, source, "", tcpOptions);
    
    if(recv_packet.tcp->rst == 1){
        progressBar_print(100, "received reset packet");
        return NULL;
    }else if(recv_packet.tcp->psh != 1 && recv_packet.tcp->ack != 1){
        progressBar_print(100, "unkown flag condition!!");
        return NULL;
    }
    free(recv_packet.pBuffer);
    progressBar_print(40, "received request details--");
        

    //send data/response
    // seq = ntohl(recv_packet.tcp.ack_seq);
    // ack_seq = ntohl(recv_packet.tcp->seq) + 1; //add one for the syn flag
    sprintf(tcpOptions, "pa\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, hints->local_port, hints->remote_port, dest, source, predefinedResponse, tcpOptions);
    //TODO: instead of predefinedResponse, send response according to request

    progressBar_print(50, "sent response to client");

    listenForPacket(&recv_packet, recv_socket, protocol, (char *) &hints->LocalIpAddr, hints->local_port, (char *) &hints->RemoteIpAddr, hints->remote_port,  seq+=strlen(predefinedResponse));
    if(recv_packet.tcp->ack == 1) //normal behavior;
    free(recv_packet.pBuffer);

    progressBar_print(70, "finished data exchange, going to end tcp stream");

    //fin, psh, ack
    sprintf(tcpOptions, "fpa\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, hints->local_port, hints->remote_port, dest, source, "", tcpOptions);
    seq+=1;

    //listen for last fin, ack
    listenForPacket(&recv_packet, recv_socket, protocol, (char *) &hints->LocalIpAddr, hints->local_port, (char *) &hints->RemoteIpAddr, hints->remote_port, seq);
    if(recv_packet.tcp->ack == 1 && recv_packet.tcp->fin == 1) //normal behavior;
    if(recv_packet.tcp->fin == 1) ack_seq++;
    free(recv_packet.pBuffer);

    sprintf(tcpOptions, "a\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, hints->local_port, hints->remote_port, dest, source, "", tcpOptions);
    

    progressBar_print(100, "[%d] Finished, sent data: %s", handledCount, hints->RemoteIpAddr);
    if(verbose){
        printf("\n");
    }else{
        if(animation){
            printf("\33[2K\r[-] server open and waiting for requests [-]   (handled: %d)", handledCount);
        }
    }

    close(recv_socket);
    return NULL;
}

int tcp_request_singleThread(int send_socket, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, char* requestMsg, char* finalMsg){
    int protocol = getProtocol(checksumType);
    progressBar_print(1, "started tcp request");
    Rsock_packet recv_packet;

    progressBar_print(10, "client: %d | %s", src_port, src_ip);

    struct addrinfo *dest, *source;
    int error = getaddrinfo(dest_ip, NULL, &addr_settings, &dest);
    error = getaddrinfo(src_ip, NULL, &addr_settings, &source);
    if (error) {
        printf("Destination ip invalid: %s\n", dest_ip);
        return -1;
    }

    //initialize random sequence number
    uint32_t seq = rand() % 1000000000;
    uint32_t ack_seq = 0;
    char tcpOptions[50];

    //setup receive send_socket first so it won't take time in between the sending and receiving and miss the packet
    int recv_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    progressBar_print(20, "initiating tcp handshake...");
    /**
     * First Packet  ------
     */
    sprintf(tcpOptions, "s\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

    progressBar_print(25, "waiting response...");

    /**
     * Receive syn&ack Packet  ------
     */
    listenForPacket(&recv_packet, recv_socket, protocol, src_ip, src_port,dest_ip, dest_port,  seq+=1);
    ack_seq = ntohl(recv_packet.tcp->seq) + recv_packet.payload_len;
    if(recv_packet.tcp->syn == 1) ack_seq++;
    // //send acknowledge first

    if(recv_packet.tcp->rst == 1){
        progressBar_print(100, "tcp received reset flag, stopped process");
        return -1;
    }else if (recv_packet.tcp->syn != 1 && recv_packet.tcp->ack != 1){
        progressBar_print(100, "unkown flag condition!!");
        return -1;
    }
    free(recv_packet.pBuffer);

    sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

    progressBar_print(30, "received, fin 3-way handshake");
    //END: finish handshake w/ vv --------------------------------------------------------------------------------------------

    /**
     * send "Third" Packet  ------
     */
    // seq = ntohl(recv_packet.tcp.ack_seq);                                            //WARNING: remove not tested yet
    // ack_seq = ntohl(recv_packet.tcp.seq) + 1; //add one for the syn flag             //WARNING: remove not tested yet

    sprintf(tcpOptions, "pa\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, requestMsg, tcpOptions);

    progressBar_print(39, "sent request string");


    // /**
    //  * Receive ack Packet  ------
    //  */
    listenForPacket(&recv_packet, recv_socket, protocol, src_ip, src_port, dest_ip, dest_port, seq+=strlen(requestMsg));
    if(recv_packet.tcp->ack == 1) //normal behavior; TODO: else and handle exception
    

    /**
     * Receive data with ack Packet  ------ pa
     */
    listenForPacket(&recv_packet, recv_socket, protocol,  src_ip, src_port,dest_ip, dest_port, seq);
    if(recv_packet.tcp->ack == 1) //normal behavior; TODO: else and handle exception
    //send acknowledge first
    ack_seq = ntohl(recv_packet.tcp->seq) + recv_packet.payload_len; //
    if(recv_packet.tcp->syn == 1) ack_seq++;
    sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

    strncpy(finalMsg, (char *) recv_packet.payload, recv_packet.payload_len);

    progressBar_print(39, "received ack packet with the data");
    free(recv_packet.pBuffer);
    // progressBar_print(61, "received message from server");

    /**
     * Receive remaining Packets  ------
     */
    listenForPacket(&recv_packet, recv_socket, protocol, src_ip, src_port, dest_ip, dest_port, seq);
    if(recv_packet.tcp->fin == 1){
        ack_seq++;
        free(recv_packet.pBuffer);
        sprintf(tcpOptions, "fa\t%u\t%u\t", seq, ack_seq);
        tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

        listenForPacket(&recv_packet, recv_socket, protocol, src_ip, src_port, dest_ip, dest_port, seq);
        if(recv_packet.tcp->ack == 1) //normal behavior; TODO: else and handle exception
        free(recv_packet.pBuffer);
    }else{
        //TODO: keep adding to the finalMsg or react to the flags
        free(recv_packet.pBuffer);
    }



    progressBar_print(100, "[%d] Finished:: received message from server", handledCount);
    if(verbose){
        printf("\n");
    }

    close(recv_socket);
    // end_progressBar(0);
    return 1;
}

struct tcpArgs{
    int send_socket;
    char* dest_ip;
    int dest_port;
    char* src_ip;
    int src_port;
    char* requestMsg;
    char* finalMsg;
    pthread_cond_t done;
};

void* tcp_request_pass(void* argsParam){
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    
    struct tcpArgs* args = (struct tcpArgs*) argsParam;
    tcp_request_singleThread(args->send_socket, args->dest_ip, args->dest_port, args->src_ip, args->src_port, args->requestMsg, args->finalMsg);

    pthread_cond_signal(&args->done);
    return NULL;
}

int tcp_request_wTimeout(int timeoutSec, int send_socket, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, char* requestMsg, char* finalMsg){
    struct tcpArgs args = {
        send_socket,
        dest_ip,
        dest_port,
        src_ip,
        src_port,
        requestMsg,
        finalMsg,
        PTHREAD_COND_INITIALIZER
    };
    pthread_mutex_t inProcess = PTHREAD_MUTEX_INITIALIZER;
    pthread_t thread_id;
    int success = 1;

    pthread_mutex_lock(&inProcess);
    struct timespec abs_time;
    clock_gettime(CLOCK_REALTIME, &abs_time);
    abs_time.tv_sec += timeoutSec;

    pthread_create(&thread_id, NULL, tcp_request_pass, &args);

    int ifTimeout = pthread_cond_timedwait(&args.done, &inProcess, &abs_time);
    if (ifTimeout == ETIMEDOUT){
        if(verbose)     progressBar_print(100, "(-1) TIMEOUT: tcp request timeout (%ds): %s\n", timeoutSec, dest_ip);
        else            printf("tcp request timeout   || \n");


        if(recordDB){
            db_put((void *) "---,---,---,---,---,---", 1);
            db_put((void *) "---,---,---,---,---,---", 2);
        }
        
        success = -1;
    }

    pthread_mutex_unlock(&inProcess);
    pthread_cancel(thread_id);
    pthread_join(thread_id, NULL);
    return success;
}

struct tcpHandleRequest_args{
    void* passArgs;
    pthread_cond_t done;
};


void* tcpHandleRequest_pass(void* vargp){
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    
    struct tcpHandleRequest_args* args = (struct tcpHandleRequest_args*) vargp;
    tcpHandleRequest_singleThread(args->passArgs);

    pthread_cond_signal(&args->done);
    return NULL;
}

void* tcpHandleRequest_wTimeout(void* vargp){
    struct tcpHandleRequest_args args = {
        vargp,
        PTHREAD_COND_INITIALIZER
    };
    pthread_mutex_t inProcess = PTHREAD_MUTEX_INITIALIZER;
    pthread_t thread_id;

    pthread_mutex_lock(&inProcess);
    struct timespec abs_time;
    clock_gettime(CLOCK_REALTIME, &abs_time);
    abs_time.tv_sec += 10;

    pthread_create(&thread_id, NULL, tcpHandleRequest_pass, &args);

    int ifTimeout = pthread_cond_timedwait(&args.done, &inProcess, &abs_time);
    pthread_cancel(thread_id);
    if (ifTimeout == ETIMEDOUT){
        if(verbose){
            progressBar_print(100, "(-1) TIMEOUT: tcp request timeout (%ds): %s\n", 10, ((Packet_hint_pointers *) vargp)->RemoteIpAddr);
        }else{
            printf("  tcp response timeout   || ");
            fflush(stdout);
        }

        if(recordDB){
            db_put((void *) "---,---,---,---,---,---", 1);
            db_put((void *) "---,---,---,---,---,---", 2);
        }
    }

    pthread_mutex_unlock(&inProcess);
    pthread_join(thread_id, NULL);
    *((volatile int*)((Packet_hint_pointers*) vargp)->expansion_ptr) = 2;
    return NULL;
}

int constructTcpHeader(struct tcp_packet* tBufferPacket, int protocol, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions){
    int headersize;
    int packetsize;
    int tcpHeaderLen;
    uint32_t seq;
    uint32_t seq_ack;

    //parse options from string because of the how long the parameter is
    int optionLen;
    if(pOptions != NULL) optionLen = strlen(pOptions);
    else optionLen = 0;

    char options[optionLen];
    strncpy(options, pOptions, optionLen);
    //separate string for char options
    char *pParam = strchr(options, '\t');
    if (pParam != NULL) memset(pParam, '\0', 1);

    tcpHeaderLen = 20;
    headersize = sizeof(tBufferPacket->ip) + tcpHeaderLen;
    packetsize = headersize + strlen(message);

    tBufferPacket->ip.ip_v = 4;
    tBufferPacket->ip.ip_hl = sizeof(tBufferPacket->ip) >> 2;
    tBufferPacket->ip.ip_dst = ((struct sockaddr_in *) dest->ai_addr)->sin_addr;
    tBufferPacket->ip.ip_src = ((struct sockaddr_in *) source->ai_addr)->sin_addr;
    tBufferPacket->ip.ip_p = protocol;
    tBufferPacket->ip.ip_ttl = 23;
    tBufferPacket->ip.ip_len = htons(packetsize);

    if(checksumType == 2){
        tBufferPacket->ip.ip_sum = 0;
        tBufferPacket->ip.ip_sum = csum((void *) tBufferPacket, packetsize);
    }

	//Ip checksum

    tBufferPacket->tcp.source = htons(src_port);
    tBufferPacket->tcp.dest = htons(dst_port);
    tBufferPacket->tcp.doff = tcpHeaderLen/4;
    tBufferPacket->tcp.res1 = 0;
    tBufferPacket->tcp.res2 = 0;

    //options
    if(strchr(pOptions, 'f') != NULL) tBufferPacket->tcp.fin = 1;
    else tBufferPacket->tcp.fin = 0;
    if(strchr(pOptions, 's') != NULL) tBufferPacket->tcp.syn = 1;
    else tBufferPacket->tcp.syn = 0;
    if(strchr(pOptions, 'r') != NULL) tBufferPacket->tcp.rst = 1;
    else tBufferPacket->tcp.rst = 0;
    if(strchr(pOptions, 'p') != NULL) tBufferPacket->tcp.psh = 1;
    else tBufferPacket->tcp.psh = 0;
    if(strchr(pOptions, 'a') != NULL) tBufferPacket->tcp.ack = 1;
    else tBufferPacket->tcp.ack = 0;
    if(strchr(pOptions, 'u') != NULL) tBufferPacket->tcp.urg = 1;
    else tBufferPacket->tcp.urg = 0;

    //rest of options
    seq = 1;
    seq_ack = 0;
    if(pParam != NULL){
        char *pSecParam = strchr(pParam + 1, '\t');
        if(pSecParam != NULL){
            memset(pSecParam, '\0', 1);
            seq = strtol(pParam + 1, NULL, 10);
            char *pThirdParam = strchr(pSecParam + 1, '\t');
            if(pThirdParam != NULL){
                memset(pThirdParam, '\0', 1);
                seq_ack = strtol(pSecParam + 1, NULL, 10);
            }
        }
    }

    tBufferPacket->tcp.seq = htonl(seq);
    tBufferPacket->tcp.ack_seq = htonl(seq_ack);
    tBufferPacket->tcp.window = htons(65535);
    tBufferPacket->tcp.check = 0;
    tBufferPacket->tcp.urg_ptr = 0;

    memset(tBufferPacket->payload, '\0', PAYLOAD_MAX_LEN);
    strcpy(tBufferPacket->payload, message);

    return packetsize;
}


 #define DESTMAC0	0x78
 #define DESTMAC1	0x92
 #define DESTMAC2	0x9c
 #define DESTMAC3	0xeb
 #define DESTMAC4	0x3f
 #define DESTMAC5	0x3e

//if success return packet length
int tcp_sendPacket(int sock, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions){
    //construct header
    int protocol = getProtocol(checksumType);
    int packetsize;

    if(checksumType == 2) {
        struct eth_packet outerBufferPacket;
        packetsize = constructTcpHeader(
            &outerBufferPacket.iptcp, 
            protocol, 
            src_port, 
            dst_port, 
            dest, 
            source, 
            message, 
            pOptions
        );

        outerBufferPacket.iptcp.tcp.check = compute_tcpcrc_checksum((void *) &outerBufferPacket + sizeof(struct ethhdr));

        outerBufferPacket.eth.h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]);
        outerBufferPacket.eth.h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]);
        outerBufferPacket.eth.h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]);
        outerBufferPacket.eth.h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]);
        outerBufferPacket.eth.h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]);
        outerBufferPacket.eth.h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]);

        outerBufferPacket.eth.h_dest[0]    =  DESTMAC0;
        outerBufferPacket.eth.h_dest[1]    =  DESTMAC1;
        outerBufferPacket.eth.h_dest[2]    =  DESTMAC2;
        outerBufferPacket.eth.h_dest[3]    =  DESTMAC3;
        outerBufferPacket.eth.h_dest[4]    =  DESTMAC4;
        outerBufferPacket.eth.h_dest[5]    =  DESTMAC5;

        outerBufferPacket.eth.h_proto = htons(ETH_P_IP);   //0x800
        
        struct sockaddr_ll sadr_ll;
        sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex;
        sadr_ll.sll_halen   = ETH_ALEN;
        sadr_ll.sll_addr[0]  = DESTMAC0;
        sadr_ll.sll_addr[1]  = DESTMAC1;
        sadr_ll.sll_addr[2]  = DESTMAC2;
        sadr_ll.sll_addr[3]  = DESTMAC3;
        sadr_ll.sll_addr[4]  = DESTMAC4;
        sadr_ll.sll_addr[5]  = DESTMAC5;

        
        printf("I'm here\n");
        fflush(stdout);
	    int send_len = sendto(sock, &outerBufferPacket, 64, 0, (const struct sockaddr*)&sadr_ll, sizeof(struct sockaddr_ll));
        
		if(send_len<0){
			printf("error in sending....sendlen=%d....errno=%d\n",send_len,errno);
			return -1;

		}

    }else{
        struct tcp_packet tBufferPacket;
        packetsize = constructTcpHeader(
            &tBufferPacket, 
            protocol, 
            src_port, 
            dst_port, 
            dest, 
            source, 
            message, 
            pOptions
        );
        tBufferPacket.tcp.check = compute_tcp_checksum_withFullPacket((void *) &tBufferPacket);
        // printf("validate: %d\n", validate_tcp_checksum((struct iphdr *) &tBufferPacket.ip, (unsigned short *) &tBufferPacket.tcp));

        int success = sendto(sock, &tBufferPacket, packetsize, 0, dest->ai_addr, dest->ai_addrlen);
        if (success < 0){
            // printf("ERROR SENDING PACKET\n");
            return -1;
        }

        if(recordDB){
            char jsonSubmit[99999];
            //record to database
            char* packetBinSeq = toBinaryString((void *) &tBufferPacket, packetsize);
            char* dataBinSeq = toBinaryString((void *) message, strlen(message));
            char* timestamp = getTimestamp();
                        
            if(recordJson) sprintf(jsonSubmit, "{\"tableName\": \"packet_sent\", \"ifAuto\": true, \"seq\": %u, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", tBufferPacket.tcp.seq, dataBinSeq, packetBinSeq, timestamp);
            else sprintf(jsonSubmit, "\"packet_sent\",true,%u,\"%s\",\"%s\",\"%s\"", tBufferPacket.tcp.seq, dataBinSeq, packetBinSeq, timestamp);

            db_put((void *) jsonSubmit, 1);

            free(packetBinSeq);
            free(dataBinSeq);
            free(timestamp);
        }
    }


    return packetsize;
}



unsigned short compute_tcp_checksum_withFullPacket(void* fullPacket) {
    struct iphdr *pIph = (struct iphdr *) fullPacket; //-Wall warning for packed struct, suppressed with void* pointer, Probably fixed, but not 100% sure
    unsigned short *ipPayload = (unsigned short *) (fullPacket + sizeof(struct iphdr));
    register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    //add the pseudo header 
    //the source ip -> dest ip -> protocol -> tcpLength
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    sum += htons(IPPROTO_TCP);
    sum += htons(tcpLen);

    //initialize checksum to 0
    struct tcphdr* tcphdr = (struct tcphdr*)(ipPayload);
    tcphdr->check = 0;

    //add the IP payload
    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }    
    //if any bytes left, pad the bytes and add
    if(tcpLen > 0) {
        // printf("+++++++++++padding, %dn", tcpLen);
        sum += ((*ipPayload)&htons(0xFF00));
    }

    //turn 32-bit sum to 16-bit: add carrier to result
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;

    return (unsigned short)sum;
}

//TODO: cleanup , straight copied from v1

#define CRC16 0x8005

unsigned short compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
    register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    //add the pseudo header 
    //the source ip -> dest ip -> protocol -> tcpLength
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    sum += htons(IPPROTO_TCP);
    sum += htons(tcpLen);

    //initialize checksum to 0
    struct tcphdr* tcphdr = (struct tcphdr*)(ipPayload);
    tcphdr->check = 0;

    //add the IP payload
    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }    
    //if any bytes left, pad the bytes and add
    if(tcpLen > 0) {
        // printf("+++++++++++padding, %dn", tcpLen);
        sum += ((*ipPayload)&htons(0xFF00));
    }

    //turn 32-bit sum to 16-bit: add carrier to result
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;

    return (unsigned short)sum;
}

//modified from: https://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum
unsigned short compute_tcpcrc_checksum(void* fullPacket) {
    struct iphdr *pIph = (struct iphdr *) fullPacket; //-Wall warning for packed struct, suppressed with void* pointer, Probably fixed, but not 100% sure
    unsigned short *ipPayload = (unsigned short *) (fullPacket + sizeof(struct iphdr));
    // register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    //initialize checksum in the tcp header to 0
    struct tcphdr* tcphdr = (struct tcphdr*)(ipPayload);
    tcphdr->check = 0;

    //add the pseudo header 
    //combining all the data with the "header header" at the bottom
    char finalData[tcpLen + 6];
    strncpy(finalData, (char *)tcphdr, tcpLen);
    
    char transferBuffer[2];
    int tcpCrcProtocol = 206; //customly defined protocol
    //adding - the source ip -> dest ip -> protocol -> tcpLength
    strncpy(transferBuffer, (char *) &pIph->saddr, 2);
    strncat(finalData, transferBuffer, 2);
    strncpy(transferBuffer, (char *) &pIph->daddr, 2);
    strncat(finalData, transferBuffer, 2);
    uint8_t numberBuffer = htons(tcpCrcProtocol);
    strncpy(transferBuffer, (char *) &numberBuffer, 1);
    strncat(finalData, transferBuffer, 1);
    numberBuffer = htons(tcpLen);
    strncpy(transferBuffer, (char *) &numberBuffer, 1);
    strncat(finalData, transferBuffer, 1);

    int dataLength = tcpLen + 6; //additional 12 8-bits(char) added to the data to be go through crc
    dataLength *= 2; //times 2 because the calculation is incremented by 8 bits, instead of the 16 bits of the char variable that the length is representing

    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    const uint8_t *data = (uint8_t *) &finalData;
    //run the data bits through the CRC loop starting from the least significant bit instead of from the most significant bit
    while(dataLength > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1;

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            dataLength--;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;
    }

    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
}

unsigned short validate_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
    register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    //add the pseudo header 
    //the source ip -> dest ip -> protocol -> tcpLength
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    sum += htons(IPPROTO_TCP);
    sum += htons(tcpLen);

    /**
     *  For Debugging, print out tcp packet in hex into console 
    
    int i;
    unsigned char *pointer = (unsigned char *)ipPayload;
    for (i = 0; i < tcpLen; i++) {
        printf("%02x ", pointer[i]);
    }
    printf("\n");
    */

    //Does not turn checksum to 0 -> to validate the sum will be 0

    //add the IP payload
    // printf("ipPayload csum: %d\n", tcpLen);
    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }    
    //if any bytes left, pad the bytes and add
    if(tcpLen > 0) {
        // printf("+++++++++++padding, %dn", tcpLen);
        sum += ((*ipPayload)&htons(0xFF00));
    }

    //turn 32-bit sum to 16-bit: add carrier to result
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;

    //set computation result
    return (unsigned short) sum;
}


/*
	Generic checksum calculation function
*/
unsigned short csum(void *packetPtr,int nbytes) 
{
    unsigned short* ptr = packetPtr;
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
}