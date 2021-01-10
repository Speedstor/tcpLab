#include "./main.h"
#include<sys/ioctl.h>

//global variables
#include "../global/global.h"

void usage();
void setupGlobalEth(int sock_raw);

int main(int argc, char **argv) {
    //setting variables -------------
    memset(&addr_settings, 0, sizeof(addr_settings));
    addr_settings.ai_flags = AF_INET;
    addr_settings.ai_socktype = SOCK_RAW;
    addr_settings.ai_protocol = IPPROTO_RAW;

    cycle_running = 0;
    in_progress = 0;
    recordDB = 0;
    ifMultithread = 0;
    verbose = 0;
    recordJson = 0;
    animation = 1;
    checksumType = 1;
    /* checksumType
            1 -> normal tcp checksum
            2 -> crc
            3 -> none
        */

    handledCount = 0;

    //a store of addresses to keep track of what is of interest and in store
    Packet_hint_pointers focusedAddrses[SERVER_MAX_CONNECTIONS];                       
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
        "velit scelerisque in dictum non consectetur a erat nam at lectus urna duis convallis convallis tellus id interdum velit laoreet id donec ultrices tincidunt arcu non sodales neque sodales ut etiam sit amet nisl purus in mollis nunc sed id semper risus in hendrerit gravida rutrum quisque non tellus orci ac auctor augue mauris augue neque gravida in fermentum et sollicitudin ac orci phasellus egestas tellus rutrum tellus pellentesque eu tincidunt tortor aliquam nulla facilisi cras fermentum odio eu feugiat pretium nibh ipsum consequat nisl vel pretium lectus quam id leo in vitae turpis massa sed elementum tempus egestas sed sed risus pretium quam vulputate dignissim suspendisse in est ante in nibh mauris cursus mattis molestie a iaculis at erat pellentesque adipiscing commodo elit at imperdiet dui accumsan sit amet nulla facilisi morbi tempus iaculis urna id volutpat lacus laoreet",    //message[PAYLOAD_MAX_LEN];
        -1
    };

    srand(time(0)); 
    
    //receive args -------------
    char c;
    while (1) {
        c = getopt_long(argc, argv, "ADjvMhrSRBP:s:d:p:i:m:c:", NULL, NULL);
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
        case 'c': //checksumType
            checksumType = strtol(optarg, NULL, 10);
            if(checksumType <= 0) checksumType = 1;
            if(checksumType >= 3) checksumType = 2;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'M':
            printf("multithread not supported in sending yet, but the functional thread is functional in receiving packets (meaning its there but its useless)\n");
            ifMultithread = 1;
            break;
        case 'D':
            animation = 0;
            break;
        case 'A':
            animation = 1;
            break;
        case 'j':
            recordJson = 1;
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

    settings.protocol = getProtocol(checksumType);

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
    if(checksumType == 1) settings.sendSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    else if(checksumType == 2) {
        settings.sendSocket = socket(AF_PACKET,SOCK_RAW,IPPROTO_RAW);
        setupGlobalEth(settings.sendSocket);
    }
	settings.receiveSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (settings.sendSocket < 0 || settings.receiveSocket < 0) {
        printf("socket opening error:: !might need root privileges\n");
        return -1;
    }

    //setup command thread
    pthread_t commandThread_id;
    pthread_create(&commandThread_id, NULL, commandThread, &settings);

    ReceiveThread_args rArgs = { &focusedAddrses, &settings };
    pthread_t receiveThread_id;
    pthread_t serverThread_id;
    if(ifMultithread){
        //setup receive thread 
        pthread_create(&receiveThread_id, NULL, receiveThread, &rArgs);
    }

    if(settings.receive_mode){
        pthread_create(&serverThread_id, NULL, serverThread, &rArgs);
    }

    //stall
    printf("program initialized, stalling... (1): --\n");

    char loadingChars[4] = "\\|/-";
    int loadIndex = 0;
    while(running){
        if(animation){
            if(settings.receive_mode){
                printf("\r[%c] server open and waiting for requests [%c]", loadingChars[loadIndex],  loadingChars[loadIndex]);
                fflush(stdout);
                loadIndex++;
                if(loadIndex >= 4) loadIndex = 0;
            }
        }
        sleep(1);
    }
    pthread_join(commandThread_id, NULL);
    if(receiveThread_id) pthread_join(receiveThread_id, NULL);
    if(serverThread_id) pthread_join(serverThread_id, NULL);

    //for debugging re-do make
    printf("modified");
}

void usage(){
    printf("---- Tcp/ip Tester --------------------------[help]--\n\
  -i\t     network interface card that is going to be used (default*: wlp1s0)\n\
  -s\t     source ip IPv4 address (default: <from chosen device>)\n\
  -d\t     destination ip IPv4 address (default: 127.0.0.1)\n\
  -P\t     destination port number (default: 8900)\n\
  -S/R/B\t     select send or receive mode, B is for both (default: -s)\n\
  -r\t     if record packets to database\n\
  -c\t     checksum type (1: tcp, 2: crc), automatically changes protocol: (tcp: 6, crc: 206)\n\
  -M\t     multithread: unifed receive (have performance gain) [not implemented]\n\
  -m\t     message, payload of the data that going to be sent\n\
  -p\t     transport network protocol to send data\n\
  -v\t     verbose, choose if there is progress bars\n\
  -j\t     json format for recording, choose between json and csv (default is csv)\n\
  -A/D\t     animation/dull, choose if there's spinning loading chars\n\
  -h/?\t     show help\n\
"); //* means should be, for debugging it is actually ens33

}

void setupGlobalEth(int sock_raw){
    
	memset(&ifreq_i,0,sizeof(ifreq_i));
	strncpy(ifreq_i.ifr_name,"wlp0s20f3",IFNAMSIZ-1);

	if((ioctl(sock_raw,SIOCGIFINDEX,&ifreq_i))<0)
		printf("error in index ioctl reading");

        
	memset(&ifreq_c,0,sizeof(ifreq_c));
	strncpy(ifreq_c.ifr_name,"wlp0s20f3",IFNAMSIZ-1);

	if((ioctl(sock_raw,SIOCGIFHWADDR,&ifreq_c))<0)
		printf("error in SIOCGIFHWADDR ioctl reading");

	printf("Mac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]));

    

}