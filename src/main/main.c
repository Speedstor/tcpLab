#include "./main.h"

//global variables
#include "../global/global.h"

void usage();

int main(int argc, char **argv) {
    //setting variables -------------
    struct addrinfo addr_settings;
    memset(&addr_settings, 0, sizeof(addr_settings));
    addr_settings.ai_flags = AF_INET;
    addr_settings.ai_socktype = SOCK_RAW;
    addr_settings.ai_protocol = IPPROTO_RAW;

    cycle_running = 0;
    in_progress = -1;

    //a store of addresses to keep track of what is of interest and in store
    // struct packet_hint_pointers focusedAddrses[MAX_CONNECTIONS];                       TODO: add back later5
    // memset(&focusedAddrses, 0, sizeof(focusedAddrses)); //not tested !!! maybe bug

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
        c = getopt_long(argc, argv, "SRP:s:d:p:i:hm:", NULL, NULL);
        if (c == -1) break;

        switch (c) {
        case 'd': //destination ip
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
        case 'p': //default protocol that is going to be used
            settings.protocol = strtol(optarg, NULL, 10);
            if (settings.protocol != 6 && settings.protocol != 17 && settings.protocol != 206 && settings.protocol != 207) {
                printf("Invalid protocol: %d\n Valid protocols: 6(tcp), 17(udp), 206(custom_tcp), 217(custom_udp)\n", settings.port);
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
    //setup send socket
    send_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (send_socket < 0) {
        printf("socket opening error\n");
        return -1;
    }else printf("send socket opened fine (1): program initialized--");

    //setup command thread
    pthread_t commandThread_id;
    pthread_create(&commandThread_id, NULL, commandThread, &settings);

    //setup receive thread
    

    //stall
    while(running){
        
    }
}

void usage(){
    printf("---- Tcp/ip Tester --------------------------[help]--\n\
  -r\t     record packets to database\n\
  -u\t     url to send recorded packets to\n\
  -S/R\t     select send or receive mode (default: -s)\n\
  -P\t     destination port number (default: 8900)\n\
  -s\t     source ip IPv4 address (default: <from chosen device>)\n\
  -d\t     destination ip IPv4 address (default: 127.0.0.1)\n\
  -i\t     network interface card that is going to be used (default*: wlp1s0)\n\
  -m\t     message, payload of the data that going to be sent\n\
  -p\t     default protocol to send data\n"); //* means should be, for debugging it is actually ens33
}