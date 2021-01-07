#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>

#include "./definitions.h"
#include "../global/global.h"
#include "./spz.h"
#include "../socket/socketCore.h"

void* cycleSendThread(void* vargp);

void printSettings(Settings_struct printSetting){
    printf("\n--- Tcp Lab (send custom udp/mostly tcp socket) ------------------------------------------------------------------\n\
\n\
   Startup Params:  port: %d | send_mode: %d | receive_mode: %d | dest ip: %s | \n\
                     net interface: %s | source ip: %s | source port: %d | protocol: %d\n\
                     message[%d]: %s\n\
\n\
-----------------------------------------------------------------------------------------------------------\n\n\n",
    printSetting.port, printSetting.send_mode, printSetting.receive_mode, printSetting.dest_ip, printSetting.network_interface, printSetting.source_ip, printSetting.src_port, printSetting.protocol, (int)strlen(printSetting.message), printSetting.message);
}

void commandUsage(){
    printf("\
----[help]---------------------------[1/1]---\n\
   help\t   show help\n\
   exit\t   stop program\n\
   _\t   send w/ default set paramter\n\
   msg [s/f]\t   set message from string or from file, the follow up param is the input\n\
   dest [arg]]\t   set the destination ip to send to\n\
   periodic\t   send request for data every 5 seconds\n\
   emptyDB\t   empty the database that records the packets\n\
   send <prot>\t   send packet with the protocol specified\n\
");
}

void* commandThread(void* vargp) {
    Settings_struct* sets = vargp;

    char command[100];
    while (1) {
        fgets(command, 100, stdin);
        command[strcspn(command, "\n")] = '\0';
        char *pParam = strchr(command, ' ');
        if (pParam != NULL) {
            memset(pParam, '\0', 1);
        }

        if (strcmp(command, "") == 0 || strcmp(command, "_") == 0 || strcmp(command, "send") == 0) {
            if(strcmp(command, "") == 0 && sets->send_mode != 1) continue;
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
                    clientRequest(sets->sendSocket, protocol, sets->source_ip, sets->dest_ip, sets->port, sets->message);
                    // send_data(sets->socket, protocol, sets->port, sets->dest_ip, sets->message);                 //TODO: add back later
                }
            }
            else
            {
                clientRequest(sets->sendSocket, sets->protocol, sets->source_ip, sets->dest_ip, sets->port, sets->message);
                // send_data(sets->socket, sets->protocol, sets->port, sets->dest_ip, sets->message);                   //TODO: add back later
            }
        }else if (strcmp(command, "msg") == 0) {
            if (pParam != NULL){
                char firstParam[100];
                char secondParam[100];
                strncpy(firstParam, pParam + 1, 100);
                char *pSecParam = strchr(firstParam, ' ');

                if(in_progress == 1){
                    printf("a thread is running and using the settings variables, will set afterwards, queing...");
                    while(in_progress == 1) msleep(200);
                }

                in_progress = 1;
                if (pSecParam != NULL) {
                    memset(pSecParam, '\0', 1);
                    strncpy(secondParam, pSecParam + 1, 100);
                    if (firstParam[0] == 's') { //string option
                        strcpy(sets->message, secondParam);
                    }
                    else if (firstParam[0] == 'f') { //file option
                        char* messageFromFile = FileToString(secondParam);
                        if(messageFromFile == NULL){
                            printf("file path given for message does not exist, or simply just error:: Given Path: %s", secondParam);
                            in_progress = -1;
                            continue;
                        } 
                        strncpy(sets->message, messageFromFile, PAYLOAD_MAX_LEN);
                        free(messageFromFile);
                    }
                    printf("message var changed to: %s\n", sets->message);
                }else {
                    printf("msg must need 2 arguments | Usage: msg [s/f] [string/fileLoc]\n");
                }
                in_progress = -1;
            }else{
                printf("msg must need arguments | Usage: msg [s/f] [string/fileLoc]\n");
            }
        }else if (strcmp(command, "dest") == 0) {
            if(pParam != NULL){
                if(in_progress == 1){
                    printf("a thread is running and using the settings variables, will set afterwards, queing...");
                    while(in_progress == 1) msleep(200);
                }
                in_progress = 1;
                memset(&sets->dest_ip, '\0', IPV4STR_MAX_LEN);
                strncpy(sets->dest_ip, pParam + 1, IPV4STR_MAX_LEN);
                printf("destination ip set to: %s\n", sets->dest_ip);
                in_progress = -1;
            }
        }else if (strcmp(command, "state") == 0 || strcmp(command, "settings") == 0 || strcmp(command, "setting") == 0) {
            printSettings(*sets);
        }else if (strcmp(command, "help") == 0) {
            commandUsage();
        }else if (strcmp(command, "exit") == 0 || strcmp(command, "stop") == 0 || strcmp(command, "abort") == 0){
            abort();
        }else if (strcmp(command, "periodic") == 0) {
            if(pParam != NULL){
                if(strcmp(pParam + 1, "false") == 0 || strcmp(pParam + 1, "off") == 0 || strcmp(pParam + 1, "end") == 0 || strcmp(pParam + 1, "stop") == 0){
                    cycle_running = 0;
                    printf("[periodic:end] ending thread that send requests--\n");
                    continue;
                }
            }
            if(cycle_running == 0) {
                cycle_running = 1;
                printf("starting thread to send request--");
                pthread_create(&cycle_thread, NULL, cycleSendThread, sets);
            }else{
                printf("[periodic] A thread is already running and sending requests\n");
            }
        }else if (strcmp(command, "endPeriodic") == 0){
            if(cycle_thread){
                pthread_cancel(cycle_thread);
                pthread_join(cycle_thread, NULL);
            }
            cycle_running = 0;
            printf("[periodic:end] ending thread that send requests--\n");
        }else if (strcmp(command, "emptyDB") == 0){
            //for clean debugging database
            if(in_progress == 1){
                printf("a thread is running and using the settings variables, will emptyDB afterwards, queing...");
                while(in_progress == 1) msleep(200);
            }
            if(cycle_running == 1){
                printf("the send cycle thread is running, will emptyDB afterwards, queing...");
                while(cycle_running == 1) msleep(200);
            }
            
            cycle_running = 1;
            in_progress = 1;
            FILE* dbFile;
            dbFile = fopen("tcpDB_sent.txt", "w");
            if(!dbFile) printf("unable to open buffer file:: error\n");
            fclose(dbFile);

            dbFile = fopen("tcpDB_receive.txt", "w");
            if(!dbFile) printf("unable to open buffer file:: error\n");
            fclose(dbFile);
            in_progress = -1;
            cycle_running = -1;

            printf("emptied database files: tcpDB_receive.txt tcpDB_sent.txt\n");
        }else if (strcmp(command, "protocol") == 0){
            if (pParam != NULL){
                if(in_progress == 1){
                    printf("a thread is running and using the settings variables, will set afterwards, queing...");
                    while(in_progress == 1) msleep(200);
                }
                in_progress = 1;
                sets->protocol = strtol((char *) pParam + 1, NULL, 10);
                printf("default protocol set to: %d\n", sets->protocol);
                in_progress = -1;
            }
        }else if (strcmp(command, "port") == 0){
            if (pParam != NULL){
                if(in_progress == 1){
                    printf("a thread is running and using the settings variables, will set afterwards, queing...");
                    while(in_progress == 1) msleep(200);
                }
                in_progress = 1;
                sets->port = strtol((char *) pParam + 1, NULL, 10);
                if (sets->protocol != 6 && sets->protocol != 17 && sets->protocol != 206 && sets->protocol != 207) {
                    printf("Invalid protocol: %d\n Valid protocols: 6(tcp), 17(udp), 206(custom_tcp), 217(custom_udp)\n", sets->port);
                }else{
                    printf("default protocol set to: %d\n", sets->port);
                }
                in_progress = -1;
            }
        }else if (strcmp(command, "clear") == 0){
            printf("\33[2J");
        }else if (strcmp(command, "animation") == 0){
            if(pParam != NULL){
                if(strcmp(pParam + 1, "false") == 0 || strcmp(pParam + 1, "off") == 0) animation = 0;
                else animation = 1;
            }else{
                animation = 1;
            }
        }else{
            printf("Invalid Command.\n");
        }
        msleep(150);
    }
}

void* cycleSendThread(void* vargp){
    Settings_struct* sets = vargp; 
    while(cycle_running == 1){
        clientRequest(sets->sendSocket, sets->protocol, sets->source_ip, sets->dest_ip, sets->port, sets->message);
        msleep(120);
    }
    cycle_running = 0;
    return NULL;
}