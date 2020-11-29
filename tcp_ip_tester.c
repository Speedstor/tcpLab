
// #include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
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
#include <pthread.h>
#include <errno.h>
#include <time.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "./includes/spz.h"
#include "./includes/rsock_recv.h"
#include "./includes/tcpHelper.h"
#include "./includes/rsockHelper.h"
#include "./includes/post.h"

#ifndef NON_STANDARD_CONSTANTS
#define NON_STANDARD_CONSTANTS
    #define IPV4STR_MAX_LEN 15
    #define PSEUDO_TCP_HDR_LEN 12
    #define NETWORK_INTERFACE_MAX_LEN 16
    #define PAYLOAD_MAX_LEN 1400
    #define MAX_CONNECTIONS 40
#endif

#ifndef PACKET_HINTS_STRUCT
#define PACKET_HINTS_STRUCT
    struct packet_hint_pointers {
        char* pSrcIpAddr;
        int src_port;
        char* pDestIpAddr;
        int dest_port;
        struct rsock_packet* pTargetSet; 
        int flag;       //DEF: 0-empty pointer || 1-wait for packet || 2-packet ready || 3-packet overwritten (warning!!)
        int sock;
        char* msg;
    };
#endif

#ifndef TCPLAB_HELPER_GLOBAL_VAR
#define TCPLAB_HELPER_GLOBAL_VAR
    struct addrinfo addr_settings;
    char source_ip[IPV4STR_MAX_LEN];
    struct packet_hint_pointers focusedAddrses[MAX_CONNECTIONS];
    char message[PAYLOAD_MAX_LEN];
#endif


void usage()
{
    printf("---- Tcp/ip Tester --------------------------[help]--\n\
  -r\t     record packets to database\n\
  -u\t     url to send recorded packets to\n\
  -S/R\t     select send or recieve mode (default: -s)\n\
  -P\t     destination port number (default: 8900)\n\
  -s\t     source ip IPv4 address (default: <from chosen device>)\n\
  -d\t     destination ip IPv4 address (default: 127.0.0.1)\n\
  -i\t     network interface card that is going to be used (default*: wlp1s0)\n\
  -m\t     message, payload of the data that going to be sent\n\
  -p\t     default protocol to send data\n"); //* means should be, for debugging it is actually ens33
}

int send_mode = 1, recieve_mode = 0;

int msleep(long msec);

void *commandThread(void *vargp);
void* pthread_raw_recv(void *vargp);
void printSettings();

// char source_ip[IPV4STR_MAX_LEN] = "";


char jsonSubmit[99999];

//default options
int port = 8900;
int protocol = 6; //default tcp
int src_port = 0; // 0 means auto
char dest_ip[IPV4STR_MAX_LEN] = "127.0.0.1";

char network_interface[NETWORK_INTERFACE_MAX_LEN] = "wlp1s0";
// char network_interface[NETWORK_INTERFACE_MAX_LEN] = "ens33";

//global variables
int sock;
pthread_t cycle_thread;
int cycle_running = 0;

int main(int argc, char **argv) {
    // sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP //required !!!!

    strcpy(message, "Speedstor -- default text message");

    srand(time(0)); 
    //setup static variables
    memset(&focusedAddrses, 0, sizeof(focusedAddrses));
    memset(&addr_settings, 0, sizeof(addr_settings));
    addr_settings.ai_flags = AF_INET;
    addr_settings.ai_socktype = SOCK_RAW;
    addr_settings.ai_protocol = IPPROTO_RAW;

    //recieve args
    char c;
    while (1)
    {
        int optionIndex = 0;
        c = getopt_long(argc, argv, "rSRP:s:d:p:i:h:m", NULL, &optionIndex);
        if (c == -1)
            break;

        switch (c)
        {
        case 'd': //destination ip
            strncpy(dest_ip, optarg, IPV4STR_MAX_LEN);
            break;
        case 'P': //port
            port = strtol(optarg, NULL, 10);
            if (port == 0)
            {
                printf("Invalid port number: %d", port);
                return -1;
            }
            break;
        case 's': //source ip
            strncpy(source_ip, optarg, IPV4STR_MAX_LEN);
            break;
        case 'm': ///message, payload
            strncpy(message, optarg, PAYLOAD_MAX_LEN);
            break;
        case 'i': //network interface card
            strncpy(network_interface, optarg, NETWORK_INTERFACE_MAX_LEN);
            break;
        case 'S': //send mode
            send_mode = 1;
            recieve_mode = 0;
            break;
        case 'R': //recieve mode
            recieve_mode = 1;
            send_mode = 0;
            break;
        case 'p': //default protocol that is going to be used
            protocol = strtol(optarg, NULL, 10);
            if (protocol != 6 && protocol != 17 && protocol != 206 && protocol != 207) {
                printf("Invalid protocol: %d\n Valid protocols: 6(tcp), 17(udp), 206(custom_tcp), 217(custom_udp)\n", port);
                return -1;
            }
            break;
        case '?': //help
        case 'h':
            usage();
            return -1;
        default:
            printf("error: the parameter that is given have error");
            return -1;
        }
    }
    
    //get sourceip and store in global variable
    if(strcmp(source_ip, "") == 0){
        char* pSourceIp = getLocalIp_s(network_interface);
        strncpy(source_ip, pSourceIp, strlen(pSourceIp) + 1);
    }

    //output the default variables that are set
    printf("\n--- Tcp Lab (send custom udp/mostly tcp socket) ------------------------------------------------------------------\n\
   \n\
   Startup Params:  port: %d | send_mode: %d | recieve_mode: %d | dest ip: %s | \n\
                     net interface: %s | source ip: %s | protocol: %d\n\
                     message[%d]: %s\n\
\n\
-----------------------------------------------------------------------------------------------------------\n\n\n",
           port, send_mode, recieve_mode, dest_ip, network_interface, source_ip, protocol, (int)strlen(message), message);
    //END: finish setting up ---------------------------------------------------------------------------------------------

    //body of the program
    pthread_t commandThread_id;
    pthread_create(&commandThread_id, NULL, commandThread, NULL);
    if (recieve_mode)
    {
        if(protocol == 6 || protocol == 206){
            printf("Recieve mode: Recieving...\n");

            pthread_t recv_pthread;
            pthread_create(&recv_pthread, NULL, pthread_raw_recv, NULL);

            int sock_r = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
            if(sock_r < 0) {
                printf("error in opening socket !!\n");
                return -1;
            }else {
                printf("socket opened fine\n");
            }

            char ipAddr[IPV4STR_MAX_LEN];
            while(1){
                struct rsock_packet* recieve;
                recieve = recievePacket(sock_r);
                
                if(strcmp(source_ip, inet_ntoa((recieve->dest).sin_addr)) == 0 && htons(recieve->tcp->dest) == port && recieve->tcp->syn == 1) {
                    //print debug text
                    char* debugText = DebugTcpHeader(recieve->tcp); 
                    printf("%s\n", debugText);
                    free(debugText);

                    //record it to the packets to be looking out for
                    strncpy(ipAddr, inet_ntoa((recieve->source).sin_addr), IPV4STR_MAX_LEN);
                    struct packet_hint_pointers hints = {(char*) &ipAddr, ntohs(recieve->tcp->source), source_ip, port, recieve};
                    pthread_t requestHandlerThread_id;
                    pthread_create(&requestHandlerThread_id, NULL, pthread_handle_request, (void *) &hints);

                    //record packet into database
                    char* packetBinSeq = toBinaryString((void *)recieve->pPacket,  sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr) + recieve->payload_len);
                    char* payloadBinSeq = toBinaryString((void *) recieve->payload, recieve->payload_len);
                    char* timestamp = getTimestamp();

                    sprintf(jsonSubmit, "{\"tableName\": \"packet_recieve\", \"ifAuto\": true, \"seq\": %d, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(recieve->tcp->seq), payloadBinSeq, packetBinSeq, timestamp);
                    async_db_put((void *) jsonSubmit, 2);
                    
                    free(packetBinSeq);
                    free(payloadBinSeq);
                    free(timestamp);
                }else{
                    free(recieve->pPacket);
                    free(recieve);
                }
            }
        }else if(protocol == 17 || protocol == 207 || protocol == 217){
            printf("Sorry, the recieve mode for protocol - (%d) is not implemented yet. The avaliable protocols with recieve mode is 6 (tcp), 206 (tcp with crc-16 checksum)", protocol);
        }else{
            printf("Sorry the protocol - (%d) is in no way implemented in this program. :(", protocol);
        }
    }else{
        printf("Send mode: Stalling...\n");

        pthread_t recv_pthread;
        pthread_create(&recv_pthread, NULL, pthread_raw_recv, NULL);
        
        //for stalling the main thread: the other two threads are command thread and the raw receive thread
        while (1){
            sleep(1000);
        }
    }

    //because of the forever loops, this should not be reached normally. TODO: maybe can break from the loops in the future instead of aborting the program.
    return 1;
}


void printSettings()
{
    //output the default variables that are set
    printf("\n--- Tcp Lab (send custom udp/mostly tcp socket) ------------------------------------------------------------------\n\
   \n\
   Startup Params:  port: %d | send_mode: %d | recieve_mode: %d | dest ip: %s | \n\
                     net interface: %s | source ip: %s | protocol: %d\n\
                     message[%d]: %s\n\
\n\
-----------------------------------------------------------------------------------------------------------\n\n\n",
           port, send_mode, recieve_mode, dest_ip, network_interface, source_ip, protocol, (int)strlen(message), message);
}

void commandUsage()
{
    if (send_mode)
    {
        printf("\
----[help]---------------------------[1/1]---\n\
   help\t   show help\n\
   exit\t   stop program\n\
   _\t   send w/ default set paramter\n\
   msg [s/f]\t   set message from string or from file, the follow up param is the input\n\
   dest [arg]]\t   set the destination ip to send to\n\
   periodic\t   send request for data every 5 seconds\n\
   emptyDB\t   empty the database that records the packets\n\
");
    }
}

void *pthread_send(void *vargp){
    send_data(sock, protocol, port, dest_ip, message);
    return NULL;
}

void *pthread_cycle_send(void *vargp){
    pthread_t send_thread;
    while(cycle_running == 1){
        pthread_create(&send_thread, 0, pthread_send, NULL);
        pthread_join(send_thread, 0);
        sleep(2);
    }
    cycle_running = 0;
    return NULL;
}

void *commandThread(void *vargp)
{
    char command[100];
    while (1)
    {
        fgets(command, 100, stdin);
        command[strcspn(command, "\n")] = '\0';
        char *pParam = strchr(command, ' ');
        if (pParam != NULL) {
            memset(pParam, '\0', 1);
        }
        if (strcmp(command, "") == 0 || strcmp(command, "_") == 0 || strcmp(command, "send") == 0)
        {
            if (pParam != NULL)
            {
                char firstParam[100];
                strncpy(firstParam, pParam + 1, 100);
                errno = 0;
                int protocol = strtol(firstParam, NULL, 10);
                if (errno || (protocol != 6 && protocol != 17 && protocol != 206 && protocol != 217))
                {
                    printf("invalid protocol, available protocols are: 6(tcp), 17(udp), 206(custom_tcp), 217(custom_udp)\n");
                }
                else
                {
                    send_data(sock, protocol, port, dest_ip, message);
                }
            }
            else
            {
                send_data(sock, protocol, port, dest_ip, message);
            }
        }
        else if (strcmp(command, "msg") == 0)
        {
            if (pParam != NULL)
            {
                char firstParam[100];
                char secondParam[100];
                strncpy(firstParam, pParam + 1, 100);
                char *pSecParam = strchr(firstParam, ' ');
                if (pSecParam != NULL)
                {
                    memset(pSecParam, '\0', 1);
                    strncpy(secondParam, pSecParam + 1, 100);
                    if (firstParam[0] == 's')
                    { //string option
                        strcpy(message, secondParam);
                    }
                    else if (firstParam[0] == 'f')
                    { //file option
                        strncpy(message, FileToString(secondParam), PAYLOAD_MAX_LEN);
                    }
                    printf("message var changed to: %s\n", message);
                }
                else
                {
                    printf("msg must need 2 arguments | Usage: msg [s/f] [string/fileLoc]\n");
                }
            }
            else
            {
                printf("msg must need arguments | Usage: msg [s/f] [string/fileLoc]\n");
            }
        }else if (strcmp(command, "dest") == 0){
            if (pParam != NULL)
            {
                memset(&dest_ip, '\0', IPV4STR_MAX_LEN);
                strncpy(dest_ip, pParam + 1, IPV4STR_MAX_LEN);
                printf("destination ip set to: %s\n", dest_ip);
            }
        }else if (strcmp(command, "state") == 0){
            printSettings();
        }else if (strcmp(command, "help") == 0){
            commandUsage();
        }else if (strcmp(command, "exit") == 0 || strcmp(command, "stop") == 0 || strcmp(command, "abort") == 0){
            abort();
        }else if (strcmp(command, "periodic") == 0) {
            if(cycle_running == 0){
                cycle_running = 1;
                printf("starting thread to send request--");
                pthread_create(&cycle_thread, NULL, pthread_cycle_send, NULL);
            }else{
                printf("A thread is already running and sending requests\n");
            }
        }else if (strcmp(command, "endPeriodic") == 0){
            cycle_running = 0;
            printf("ending thread that send requests--\n");
        }else if (strcmp(command, "emptyDB") == 0){
            //for clean debugging database
            FILE* dbFile;
            dbFile = fopen("tcpDB_sent.txt", "w");
            if(!dbFile) {
            	printf("unable to open buffer file:: error\n");
            }else{
            //   fprintf(dbFile, "\n");
            }
            fclose(dbFile);
            dbFile = fopen("tcpDB_recieve.txt", "w");
            if(!dbFile) {
            	printf("unable to open buffer file:: error\n");
            }else{
            //   fprintf(dbFile, "\n");
            }
            fclose(dbFile);

            printf("emptied database files: tcpDB_recieve.txt tcpDB_sent.txt\n");
        }else if (strcmp(command, "protocol") == 0){
            if (pParam != NULL){
                protocol = strtol((char *) pParam + 1, NULL, 10);
                printf("default protocol set to: %d\n", protocol);
            }
        }else if (strcmp(command, "port") == 0){
            if (pParam != NULL){
                port = strtol((char *) pParam + 1, NULL, 10);
                if (protocol != 6 && protocol != 17 && protocol != 206 && protocol != 207) {
                    printf("Invalid protocol: %d\n Valid protocols: 6(tcp), 17(udp), 206(custom_tcp), 217(custom_udp)\n", port);
                }else{
                    printf("default protocol set to: %d\n", port);
                }
            }
        }else{
            printf("Invalid Command.\n");
        }
    }
}


void* pthread_raw_recv(void *vargp){
    //parse vargp 
    printf("init listening thread - standby (1)\n");

	int sock_r = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sock_r < 0) {
		printf("error in opening socket !!!\n");
		return NULL;
	}else {
        // printf("socket opened fine\n");
    }

	while(1){
        struct rsock_packet* recieve; 
		recieve = recievePacket(sock_r);
        int savePacket = 0;

        if(recieve->protocol != 6) continue;

        for(int i=0; i < MAX_CONNECTIONS; i++){
            if(focusedAddrses[i].flag != 0 && focusedAddrses[i].flag != 404){
                if(strcmp(focusedAddrses[i].pSrcIpAddr, inet_ntoa((recieve->source).sin_addr)) == 0 && htons(recieve->tcp->dest) == focusedAddrses[i].dest_port 
                    && htons(recieve->tcp->source) == focusedAddrses[i].src_port && strcmp(focusedAddrses[i].pDestIpAddr, inet_ntoa((recieve->dest).sin_addr)) == 0) {

                    /* soruce ip variable in string */
                    char srcIp[IPV4STR_MAX_LEN];
                    strncpy(srcIp, focusedAddrses[i].pSrcIpAddr, IPV4STR_MAX_LEN);

                    //tcp packet
                    if(recieve->protocol == 6){
                        //checksum
                        if(validate_tcp_checksum(&recieve->ip, (unsigned short *) recieve->tcp) != 0){
                            printf("ERROR: !!recvieved tcp packet checksum invalid\n");
                            // return -1;
                        }
                        
                        //acknowledge packets, sync with the client
                        if(recieve->payload_len > 0){
                            int msgLen = recieve->payload_len;
                            if(recieve->tcp->syn == 1) msgLen++;

                            uint32_t seq = ntohl(recieve->tcp->ack_seq);
                            uint32_t ack_seq = ntohl(recieve->tcp->seq) + msgLen; //add one for the syn flag

                            char tcpOptions[50];
                            memset(&tcpOptions, '\0', 50);
                            sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
                            send_packet(focusedAddrses[i].sock, 6, focusedAddrses[i].dest_port, htons(recieve->tcp->source), srcIp, "", tcpOptions);
                        }

                        //ending packet -> finish tags
                        if(recieve->tcp->fin == 1){
                            if(send_mode == 1){
                                int msgLen = recieve->payload_len;
                                if(recieve->tcp->syn == 1) msgLen++;

                                uint32_t seq = ntohl(recieve->tcp->ack_seq);
                                uint32_t ack_seq = ntohl(recieve->tcp->seq) + msgLen + 1; //add one for the syn flag

                                char tcpOptions[50];
                                memset(&tcpOptions, '\0', 50);
                                sprintf(tcpOptions, "a\t%u\t%u\t", seq, ack_seq);
                                send_packet(focusedAddrses[i].sock, 6, focusedAddrses[i].dest_port, htons(recieve->tcp->source), srcIp, "", tcpOptions);

                                memset(&tcpOptions, '\0', 50);
                                sprintf(tcpOptions, "f\t%u\t%u\t", seq, ack_seq);
                                send_packet(focusedAddrses[i].sock, 6, focusedAddrses[i].dest_port, htons(recieve->tcp->source), srcIp, "", tcpOptions);
                            }

                            focusedAddrses[i].flag = 404; //404 waiting for acknowledge fin bit
                        }

                        //copy payload if there is any
                        if(recieve->payload_len > 0 && focusedAddrses[i].msg != NULL) {
                            strncpy(focusedAddrses[i].msg + strlen(focusedAddrses[i].msg), (char *) recieve->payload, recieve->payload_len);
                        }

                        //packets that are not purely acknowledge, save to the focusAddrses to save
                        if((recieve->payload_len > 0 || recieve->tcp->psh == 1 || recieve->tcp->syn == 1 || recieve->tcp->fin == 1 || recieve->tcp->rst == 1) && !(recieve->tcp->syn == 1 && recieve->tcp->ack == 0)){
                            //set to not free the whole variable || whatever that means
                            savePacket = 1;

                            //free the previous packet
                            if(focusedAddrses[i].flag != 1 && focusedAddrses[i].flag != 0 && focusedAddrses[i].flag != 404){
                                free(focusedAddrses[i].pTargetSet->pPacket);
                            }

                            struct rsock_packet recieveCopy = *recieve;
                            *focusedAddrses[i].pTargetSet = recieveCopy;
                            if(focusedAddrses[i].flag == 1){
                                focusedAddrses[i].flag = 2;
                            }else if(focusedAddrses[i].flag == 404){
                                
                            }else{
                                focusedAddrses[i].flag = 3;
                            }
                        }
                    }


                    //find index and update database
                    char* packetBinSeq = toBinaryString((void *)recieve->pPacket + sizeof(struct ethhdr),  sizeof(struct iphdr) + sizeof(struct tcphdr) + recieve->payload_len);
                    char* payloadBinSeq = toBinaryString((void *) recieve->payload, recieve->payload_len);
                    char* timestamp = getTimestamp();

                    sprintf(jsonSubmit, "{\"tableName\": \"packet_recieve\", \"ifAuto\": true, \"seq\": %u, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(recieve->tcp->seq), payloadBinSeq, packetBinSeq, timestamp);
                    async_db_put((void *) jsonSubmit, 2);

                    free(packetBinSeq);
                    free(payloadBinSeq);
                    free(timestamp);
                }
            }
        }

        if(savePacket == 0){
            free(recieve->pPacket);
	        free(recieve);
        }
    }
}
