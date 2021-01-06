#include "./main.h"

//global variables
#include "../global/global.h"

void usage();

int main(int argc, char **argv) {
    //setting variables -------------
    memset(&addr_settings, 0, sizeof(addr_settings));
    addr_settings.ai_flags = AF_INET;
    addr_settings.ai_socktype = SOCK_RAW;
    addr_settings.ai_protocol = IPPROTO_RAW;

    cycle_running = 0;
    in_progress = -1;
    recordDB = -1;
    ifMultithread = -1;

    //a store of addresses to keep track of what is of interest and in store
    Packet_hint_pointers focusedAddrses[MAX_CONNECTIONS];                       
    memset(&focusedAddrses, 0, sizeof(focusedAddrses));

    //setup default variables
    struct Settings_struct settings = {
        6,              //protocol
        8900,           //port
        0,              //src_port (0 means auto)
        1,              //send_mode 
        0,              //receive_mode
        "127.0.0.1",                            //dest_ip[IPV4STR_MAX_LEN]
        "",                                     //source_ip[IPV4STR_MAX_LEN] ("" means retrieve auto)
        // "wlp1s0",                               //network_interface[NETWORK_INTERFACE_MAX_LEN] (ubuntu 18.04)
        "wlp0s20f3",                            //network_interface (changed to default for debugging || wlp1s0 is the default in ubuntu 18.04)
        "Speedstor -- default text message",    //message[PAYLOAD_MAX_LEN];
        -1
    };

    srand(time(0)); 
    
    //receive args -------------
    char c;
    while (1) {
        c = getopt_long(argc, argv, "hrSRBPM:s:d:p:i:m:", NULL, NULL);
        if (c == -1) break;

        switch (c) {
        case 'd': //destination ip
            //TODO: check and document that it only supports ipv4
            strncpy(settings.dest_ip, optarg, IPV4STR_MAX_LEN);
            break;
        case 'P': //port
            settings.port = strtol(optarg, NULL, 10);
            if (settings.port == 0)
            {
                printf("Invalid port number: %d", settings.port);
                return -1;
            }
            break;
        case 's': //source ip
            strncpy(settings.source_ip, optarg, IPV4STR_MAX_LEN);
            break;
        case 'm': ///message, payload
            strncpy(settings.message, optarg, PAYLOAD_MAX_LEN);
            break;
        case 'i': //network interface card
            strncpy(settings.network_interface, optarg, NETWORK_INTERFACE_MAX_LEN);
            break;
        case 'S': //send mode
            settings.send_mode = 1;
            settings.receive_mode = 0;
            break;
        case 'R': //receive mode
            settings.receive_mode = 1;
            settings.send_mode = 0;
            break;
        case 'B':
            settings.receive_mode = 1;
            settings.send_mode = 1;
            break;
        case 'r': //record packets
            recordDB = 1;
            break;
        case 'p': //default protocol that is going to be used
            settings.protocol = strtol(optarg, NULL, 10);
            if (settings.protocol != 6 && settings.protocol != 17 && settings.protocol != 206 && settings.protocol != 207) {
                printf("Invalid protocol: %d\n Valid protocols: 6(tcp), 17(udp), 206(custom_tcp), 217(custom_udp)\n", settings.port);
                return -1;
            }
            break;
        case 'M':
            printf("multithread not supported in sending yet, but the functional thread is functional in receiving packets (meaning its there but its useless)\n");
            ifMultithread = 1;
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

    //retrieve and setup sourceip into variable
    if(strcmp(settings.source_ip, "") == 0){
        char* pSourceIp = getLocalIp_s(settings.network_interface);
        if (pSourceIp == NULL) {
            printf("Error getting device ip on interface: %s\n", settings.network_interface);
            exit(-1);
        }
        strncpy(settings.source_ip, pSourceIp, strlen(pSourceIp) + 1);
    }

    printSettings(settings);
    //END: finish setting up ---------------------------------------------------------------------------------------------

    //SECTION: start of program
    running = 1;
    //setup socket
    // settings.sendSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	settings.receiveSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (/* settings.sendSocket < 0 || */ settings.receiveSocket < 0) {
        printf("socket opening error:: !might need root privileges\n");
        return -1;
    }

    //setup command thread
    pthread_t commandThread_id;
    pthread_create(&commandThread_id, NULL, commandThread, &settings);

    ReceiveThread_args rArgs = { &focusedAddrses, &settings };
    if(ifMultithread){
        //setup receive thread 
        pthread_t receiveThread_id;
        pthread_create(&receiveThread_id, NULL, receiveThread, &rArgs);
    }

    // if(settings.receive_mode){
    //     pthread_t serverThread_id;
    //     pthread_create(&serverThread_id, NULL, serverThread, &rArgs);
    // }

    //stall
    printf("program initialized, stalling... (1): --\n");
    if(settings.receive_mode){
        while(1){
            Rsock_packet recv_packet;
            /* int success =  */receivePacket(&recv_packet, settings.receiveSocket);

            if(recv_packet.protocol != 6){
                free(recv_packet.pPacket);
                continue;
            }
            if(strcmp(settings.source_ip, inet_ntoa((recv_packet.dest).sin_addr)) == 0
               && htons(recv_packet.tcp.dest) == settings.port) {
                printf("received\n");
                fflush(stdout); 
            }
        }
        // while(1){
        //     Rsock_packet receive;
        //     receivePacket(&receive, settings.receiveSocket);

        //     if(/* strcmp(settings.source_ip, inet_ntoa((receive.dest).sin_addr)) == 0 && */ htons(receive.tcp.dest) == settings.port && receive.tcp.syn == 1) {
        //         printf("received\n");
        //         fflush(stdout);
                
        //         // //record it to the packets to be looking out for
        //         // char sendMessage[MESSAGE_MAX_LEN];
        //         // strcpy(sendMessage, settings.message);
        //         // Packet_hint_pointers hints = { "", htons(receive.tcp.source), "", settings.port, &receive, 0, settings.sendSocket, (char*) &sendMessage, 0};
        //         // strcpy(hints.RemoteIpAddr, inet_ntoa((receive.source).sin_addr));
        //         // strcpy(hints.LocalIpAddr, settings.source_ip);
        //         // pthread_t requestHandlerThread_id;
        //         // pthread_create(&requestHandlerThread_id, NULL, tcpHandleRequest_singleThread, (void *) &hints);
            
            
        //         // //record packet into database
        //         // char jsonSubmit[19999];
        //         // char* packetBinSeq = toBinaryString((void *)receive.pPacket,  sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr) + receive.payload_len);
        //         // char* payloadBinSeq = toBinaryString((void *) receive.payload, receive.payload_len);
        //         // char* timestamp = getTimestamp();

        //         // sprintf(jsonSubmit, "{\"tableName\": \"packet_receive\", \"ifAuto\": true, \"seq\": %d, \"data\": \"%s\", \"packet\": \"%s\", \"time\": \"%s\"}", ntohl(receive.tcp.seq), payloadBinSeq, packetBinSeq, timestamp);
        //         // db_put((void *) jsonSubmit, 2);
                
        //         // free(packetBinSeq);
        //         // free(payloadBinSeq);
        //         // free(timestamp);
        //     }
        //     // free(receive.pPacket);
        // }
    }
    else{
        while(running){

        }
    }

    //for debugging re-do make
    printf("modified");
}

void usage(){
    printf("---- Tcp/ip Tester --------------------------[help]--\n\
  -r\t     record packets to database\n\
  -M\t     multithread in sending and receiving (have performance gain) [not implemented in send mode yet]\n\
  -u\t     url to send recorded packets to\n\
  -S/R\t     select send or receive mode (default: -s)\n\
  -B\t     both send and receive mode\n\
  -P\t     destination port number (default: 8900)\n\
  -s\t     source ip IPv4 address (default: <from chosen device>)\n\
  -d\t     destination ip IPv4 address (default: 127.0.0.1)\n\
  -i\t     network interface card that is going to be used (default*: wlp1s0)\n\
  -m\t     message, payload of the data that going to be sent\n\
  -p\t     default protocol to send data\n"); //* means should be, for debugging it is actually ens33
}