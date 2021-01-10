#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>


FILE* dbFile_sent = NULL;
FILE* dbFile_receive = NULL;
void db_put(void* jsonPayloadP, int option){
    if(option == 1){
        if(dbFile_sent == NULL) dbFile_sent = fopen("tcpDB_sent.txt", "a");
        fprintf(dbFile_sent, "%s\n", (char *) jsonPayloadP);
    }else{
        if(dbFile_receive == NULL) dbFile_receive = fopen("tcpDB_receive.txt", "a");
        fprintf(dbFile_receive, "%s\n", (char *) jsonPayloadP);
    }

    // fclose(dbFile);
}
