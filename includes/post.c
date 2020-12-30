#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
 

#define THREAD_SIZE 50

struct MemoryStruct {
  char *memory;
  size_t size;
};

pthread_t threadsArr[THREAD_SIZE];
int threadFlag = 0;


//not actually async, changed method already, forgive me :(
void async_db_put(void* jsonPayloadP, int option){
  threadFlag++;
  if(threadFlag >= 50) threadFlag = 0;

  // printf("\n    Record   ---\n\n");

	FILE* dbFile;
  if(option == 1){
	  dbFile = fopen("tcpDB_sent.txt", "a");
  }else{
	  dbFile = fopen("tcpDB_recieve.txt", "a");
  }

	if(!dbFile) {
		printf("unable to open buffer file:: error");
	}else{
	  fprintf(dbFile, "%s\n", (char *) jsonPayloadP);
  }

  fclose(dbFile);
}
