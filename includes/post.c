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


void async_db_put(void* jsonPayloadP, int option){
  threadFlag++;
  if(threadFlag >= 50) threadFlag = 0;

  printf("\n    Record   ---\n\n");

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

  //using the web, curllib bug
  // pthread_join(threadsArr[threadFlag], NULL);
  // pthread_create(&threadsArr[threadFlag], NULL, db_put, jsonPayloadP);
}

char *stringToBinary(char *s)
{
  if (s == NULL) {
    // NULL might be 0 but you cannot be sure about it
    return NULL;
  }
  // get length of string without NUL
  size_t slen = strlen(s);

  // we cannot do that here, why?
  // if(slen == 0){ return s;}

  errno = 0;
  // allocate "slen" (number of characters in string without NUL)
  // times the number of bits in a "char" plus one byte for the NUL
  // at the end of the return value
  char *binary = malloc(slen * CHAR_BIT + 1);
  if(binary == NULL){
     fprintf(stderr,"malloc has failed in stringToBinary(%s): %s\n",s, strerror(errno));
     return NULL;
  }
  // finally we can put our shortcut from above here
  if (slen == 0) {
    *binary = '\0';
    return binary;
  }
  char *ptr;
  // keep an eye on the beginning
  char *start = binary;
  int i;

  // loop over the input-characters
  for (ptr = s; *ptr != '\0'; ptr++) {
    /* perform bitwise AND for every bit of the character */
    // loop over the input-character bits
    for (i = CHAR_BIT - 1; i >= 0; i--, binary++) {
      *binary = (*ptr & 1 << i) ? '1' : '0';
    }
  }
  // finalize return value
  *binary = '\0';
  // reset pointer to beginning
  binary = start;
  return binary;
}