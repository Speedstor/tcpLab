#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>


void db_put(void* jsonPayloadP, int option){
    FILE* dbFile;
    if(option == 1) dbFile = fopen("tcpDB_sent.txt", "a");
    else            dbFile = fopen("tcpDB_receive.txt", "a");

    if(!dbFile) {
        printf("unable to open buffer file:: error");
    }else{
        fprintf(dbFile, "%s\n", (char *) jsonPayloadP);
    }

    fclose(dbFile);
}
