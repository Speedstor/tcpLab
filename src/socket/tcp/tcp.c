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

#include "../../common/definitions.h"
#include "../../common/record.h"
#include "../../global/global.h"
#include "../packetCore.h"
#include "../../common/spz.h"

unsigned short validate_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
unsigned short compute_tcp_checksum_withFullPacket(void* fullPacket);
unsigned short compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
unsigned short compute_tcpcrc_checksum(struct iphdr *pIph, unsigned short *ipPayload);


//if success return packet length
int tcp_sendPacket(int sock, int src_port, int dst_port, struct addrinfo *dest, struct addrinfo *source, char message[PAYLOAD_MAX_LEN], char* pOptions){
    //construct header
    int protocol = 6;
    char jsonSubmit[99999];

    int headersize;
    int packetsize;
    int tcpHeaderLen;
    uint32_t seq;
    uint32_t seq_ack;

    struct tcp_packet tBufferPacket;

    int success;

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
    headersize = sizeof(tBufferPacket.ip) + tcpHeaderLen;
    packetsize = headersize + strlen(message);

    tBufferPacket.ip.ip_v = 4;
    tBufferPacket.ip.ip_hl = sizeof(tBufferPacket.ip) >> 2;
    tBufferPacket.ip.ip_dst = ((struct sockaddr_in *) dest->ai_addr)->sin_addr;
    tBufferPacket.ip.ip_src = ((struct sockaddr_in *) source->ai_addr)->sin_addr;
    tBufferPacket.ip.ip_p = protocol;
    tBufferPacket.ip.ip_ttl = 23;
    tBufferPacket.ip.ip_len = htons(packetsize);

    tBufferPacket.tcp.source = htons(src_port);
    tBufferPacket.tcp.dest = htons(dst_port);
    tBufferPacket.tcp.doff = tcpHeaderLen/4;
    tBufferPacket.tcp.res1 = 0;
    tBufferPacket.tcp.res2 = 0;

    //options
    if(strchr(pOptions, 'f') != NULL) tBufferPacket.tcp.fin = 1;
    else tBufferPacket.tcp.fin = 0;
    if(strchr(pOptions, 's') != NULL) tBufferPacket.tcp.syn = 1;
    else tBufferPacket.tcp.syn = 0;
    if(strchr(pOptions, 'r') != NULL) tBufferPacket.tcp.rst = 1;
    else tBufferPacket.tcp.rst = 0;
    if(strchr(pOptions, 'p') != NULL) tBufferPacket.tcp.psh = 1;
    else tBufferPacket.tcp.psh = 0;
    if(strchr(pOptions, 'a') != NULL) tBufferPacket.tcp.ack = 1;
    else tBufferPacket.tcp.ack = 0;
    if(strchr(pOptions, 'u') != NULL) tBufferPacket.tcp.urg = 1;
    else tBufferPacket.tcp.urg = 0;

    //rest of options
    seq = 1;
    seq_ack = 0;
    if(pParam != NULL){
        printf("%s\n", pParam);
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
    // printf("seq: %d || seq_ack: %d\n", seq, seq_ack); //for debuging use

    tBufferPacket.tcp.seq = htonl(seq);
    tBufferPacket.tcp.ack_seq = htonl(seq_ack);
    tBufferPacket.tcp.window = htons(65535);
    tBufferPacket.tcp.check = 0;
    tBufferPacket.tcp.urg_ptr = 0;

    memset(tBufferPacket.payload, '\0', PAYLOAD_MAX_LEN);
    strcpy(tBufferPacket.payload, message);

    tBufferPacket.tcp.check = compute_tcp_checksum_withFullPacket((void *) &tBufferPacket);
    // printf("validate: %d\n", validate_tcp_checksum((struct iphdr *) &tBufferPacket.ip, (unsigned short *) &tBufferPacket.tcp));

    //send tcp packet
    success = sendto(sock, &tBufferPacket, packetsize, 0, dest->ai_addr, dest->ai_addrlen);
    if (success < 0){
        // printf("ERROR SENDING PACKET\n");
        return -1;
    }

    //record to database
    char* packetBinSeq = toBinaryString((void *) &tBufferPacket, packetsize);
    char* dataBinSeq = toBinaryString((void *) message, strlen(message));
    char* timestamp = getTimestamp();

    if(recordDB){
        sprintf(jsonSubmit, "{\"tableName\": \"packet_sent\", \"seq\": %u, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", seq, dataBinSeq, packetBinSeq, timestamp);
        db_put((void *) jsonSubmit, 1);
    }

    free(packetBinSeq);
    free(dataBinSeq);
    free(timestamp);

    return packetsize;
}

int tcp_request_singleThread(int send_socket, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, char requestMsg[PAYLOAD_MAX_LEN], char* finalMsg){
    progressBar_print("started tcp request", 1);
    char debugText[150];
    Rsock_packet recv_packet;

    sprintf(debugText, "src port number %d || addr: %s\n", src_port, src_ip);
    progressBar_print(debugText, 10);

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

    progressBar_print("initiating tcp handshake................", 20);
    /**
     * First Packet  ------
     */
    sprintf(tcpOptions, "s\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

    progressBar_print("waiting response................", 25);
    

    // /**
    //  * Receive ack Packet  ------
    //  */
    // listenForPacket(&recv_packet, recv_socket, 6, dest_ip, dest_port, src_ip, src_port, seq+=1);
    // if(recv_packet.tcp.ack != 1) //normal behavior; TODO: else and handle exception

    /**
     * Receive syn&ack Packet  ------
     */
    listenForPacket(&recv_packet, recv_socket, 6, dest_ip, dest_port, src_ip, src_port, seq);
    //send acknowledge first
    ack_seq = ntohl(recv_packet.tcp.seq) + recv_packet.payload_len;
    if(recv_packet.tcp.syn == 1) ack_seq++;
    sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

    if(recv_packet.tcp.rst == 1){
        progressBar_print("tcp received reset flag, stopped process", 100);
        return -1;
    }else if (recv_packet.tcp.syn != 1 && recv_packet.tcp.ack != 1){
        progressBar_print("unkown flag condition!!", 100);
        return -1;
    }

    progressBar_print("received!! -- finished 3-way handshake -> sending actual request", 30);
    //END: finish handshake --------------------------------------------------------------------------------------------

    /**
     * send "Third" Packet  ------
     */
    // seq = ntohl(recv_packet.tcp.ack_seq);                                            //WARNING: remove not tested yet
    // ack_seq = ntohl(recv_packet.tcp.seq) + 1; //add one for the syn flag             //WARNING: remove not tested yet

    sprintf(tcpOptions, "pa\t%d\t%d\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, requestMsg, tcpOptions);

    /**
     * Receive ack Packet  ------
     */
    listenForPacket(&recv_packet, recv_socket, 6, dest_ip, dest_port, src_ip, src_port, seq+=strlen(requestMsg));
    if(recv_packet.tcp.ack == 1) //normal behavior; TODO: else and handle exception

    /**
     * Receive data Packet  ------
     */
    listenForPacket(&recv_packet, recv_socket, 6, dest_ip, dest_port, src_ip, src_port, seq);
    //send acknowledge first
    ack_seq = ntohl(recv_packet.tcp.seq) + recv_packet.payload_len; //
    if(recv_packet.tcp.syn == 1) ack_seq++;
    sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
    tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

    /**
     * Receive remaining Packets  ------
     */
    listenForPacket(&recv_packet, recv_socket, 6, dest_ip, dest_port, src_ip, src_port, seq);
    if(recv_packet.tcp.fin == 1){
        sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
        tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);

        sprintf(tcpOptions, "f\t%u\t%u\t", seq, ack_seq);
        tcp_sendPacket(send_socket, src_port, dest_port, dest, source, "", tcpOptions);
    }else{
        //TODO: keep adding to the finalMsg or react to the flags
    }

    // end_progressBar(0);
    printf("%s\n", finalMsg);
    return 1;
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

    /**
     *  For Debugging, print out tcp packet in hex into console 
    
    int i;
    unsigned char *pointer = (unsigned char *)ipPayload;
    for (i = 0; i < tcpLen; i++) {
        printf("%02x ", pointer[i]);
    }
    printf("\n");
    */

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
unsigned short compute_tcpcrc_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
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