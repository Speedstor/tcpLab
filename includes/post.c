#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
 
#include <curl/curl.h>

#define THREAD_SIZE 50

struct MemoryStruct {
  char *memory;
  size_t size;
};

pthread_t threadsArr[THREAD_SIZE];
int threadFlag = 0;
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}


void* db_put(void* jsonPayloadP) {
  char* jsonPayload = jsonPayloadP;

  CURL *curl;
  CURLcode res;

  struct MemoryStruct chunk;
 
  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
 
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://speedstor.net/tcpIp/put.php");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload);
    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself */ 
    // curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(jsonPayload));

    /* send all data to this function  */ 
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
 
 
    /* some servers don't like requests that are made without a user-agent
        field, so we provide one */ 
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);

    printf("\n%s\nPost Response: %s\n", jsonPayload, chunk.memory);
  }
}

void* async_db_put(void* jsonPayloadP, int option){
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