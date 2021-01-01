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

void commandUsage();
void printSettings(Settings_struct printSetting);

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
                    // send_data(sets->socket, protocol, sets->port, sets->dest_ip, sets->message);                 //TODO: add back later
                }
            }
            else
            {
                // send_data(sets->socket, sets->protocol, sets->port, sets->dest_ip, sets->message);                   //TODO: add back later
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
                    if (firstParam[0] == 's') { //string option
                        strcpy(sets->message, secondParam);
                    }
                    else if (firstParam[0] == 'f') { //file option
                        char* messageFromFile = FileToString(secondParam);
                        if(messageFromFile == NULL){
                            printf("file path given for message does not exist, or simply just error:: Given Path: %s", secondParam);
                            continue;
                        } 
                        strncpy(sets->message, messageFromFile, PAYLOAD_MAX_LEN);
                        free(messageFromFile);
                    }
                    printf("message var changed to: %s\n", sets->message);
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
                memset(&sets->dest_ip, '\0', IPV4STR_MAX_LEN);
                strncpy(sets->dest_ip, pParam + 1, IPV4STR_MAX_LEN);
                printf("destination ip set to: %s\n", sets->dest_ip);
            }
        }else if (strcmp(command, "state") == 0){
            printSettings(*sets);
        }else if (strcmp(command, "help") == 0){
            commandUsage();
        }else if (strcmp(command, "exit") == 0 || strcmp(command, "stop") == 0 || strcmp(command, "abort") == 0){
            abort();
        }else if (strcmp(command, "periodic") == 0) {
            if(cycle_running == 0){
                cycle_running = 1;
                printf("starting thread to send request--");
                // pthread_create(&cycle_thread, NULL, pthread_cycle_send, NULL);                                                                                           !!! Aware!!!! TODO::::
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
            dbFile = fopen("tcpDB_receive.txt", "w");
            if(!dbFile) {
            	printf("unable to open buffer file:: error\n");
            }else{
            //   fprintf(dbFile, "\n");
            }
            fclose(dbFile);

            printf("emptied database files: tcpDB_receive.txt tcpDB_sent.txt\n");
        }else if (strcmp(command, "protocol") == 0){
            if (pParam != NULL){
                sets->protocol = strtol((char *) pParam + 1, NULL, 10);
                printf("default protocol set to: %d\n", sets->protocol);
            }
        }else if (strcmp(command, "port") == 0){
            if (pParam != NULL){
                sets->port = strtol((char *) pParam + 1, NULL, 10);
                if (sets->protocol != 6 && sets->protocol != 17 && sets->protocol != 206 && sets->protocol != 207) {
                    printf("Invalid protocol: %d\n Valid protocols: 6(tcp), 17(udp), 206(custom_tcp), 217(custom_udp)\n", sets->port);
                }else{
                    printf("default protocol set to: %d\n", sets->port);
                }
            }
        }else{
            printf("Invalid Command.\n");
        }
    }
}


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
");
}