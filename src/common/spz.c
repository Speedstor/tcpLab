#include <errno.h>
#include <time.h> 
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#ifndef STATUS_BUFFER_MAX
#define STATUS_BUFFER_MAX 1200
#endif

#ifndef STATUS_BUFFER
#define STATUS_BUFFER
    char *statusBuffer[STATUS_BUFFER_MAX];
    char* lastProgressText;
#endif

struct Timestamp{
    time_t seconds;
    long milliseconds;
    char timestring[32];
};


char* getTimestamp(){
    char   timebuffer[32]     = {0};
    // struct timeval  tv        = {0};
    struct tm      *tmval     = NULL;
    struct tm       gmtval    = {0};
    struct timespec curtime   = {0};

    struct Timestamp timestamp;

    // int i = 0;

    // Get current time
    clock_gettime(CLOCK_REALTIME, &curtime);


    // Set the fields
    timestamp.seconds      = curtime.tv_sec;
    timestamp.milliseconds = round(curtime.tv_nsec/1.0e6);

    char* returnString = malloc(32);

    if((tmval = gmtime_r(&timestamp.seconds, &gmtval)) != NULL)
    {
        // Build the first part of the time
        strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d %H:%M:%S", &gmtval);

        // Add the milliseconds part and build the time string
        sprintf(returnString, "%s.%03ld", timebuffer, timestamp.milliseconds); 
    }

    // printf("%s", returnString);

    return returnString;
}

unsigned long long mTime(){
    struct timeval tv;

    gettimeofday(&tv, NULL);

    unsigned long long millisecondsSinceEpoch =
        (unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000;

    return millisecondsSinceEpoch;
}

char* toBinaryString(void* target, int size){
    char* packetBinSeq;
    packetBinSeq = malloc(999999);

    int i;
    unsigned char *pointer = (unsigned char *) target;
    for (i = 0; i < size; i++) {
        sprintf(packetBinSeq+(i*3), "%02x ", pointer[i]);
    }

    return packetBinSeq;
}

//https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

void refresh_progressBar(){
    lastProgressText = NULL;
    for(int i = 0; i < STATUS_BUFFER_MAX; i++){
        statusBuffer[i] = NULL;
    }
}


void progressBar_print(char* text, int percentage){
	char copyText[42];
	// memset(&copyText, '\0', 42);
	strncpy(copyText, text, 41);

    char progressBar[36];
    memset(&progressBar+35, '\0', 1);
    int numOfDots = (int) 35 * percentage / 100;
    for(int i = 0; i < 35; i++){
        if(i < numOfDots){
            sprintf((char*)&progressBar+i, ".");
        }else{
            sprintf((char*)&progressBar+i, " ");
        }
    }

    int i = 1;
    // for(i = 0; i < STATUS_BUFFER_MAX; i++){
    //     if(statusBuffer[i] == NULL){
    //         statusBuffer[i] = text;
    //         break;
    //     } 
    // }

    char progressText[400];
    sprintf(progressText, ">%%[%s]|%d %d%%|  %s", progressBar, i, percentage, copyText);

	int textLength = strlen(progressText);
	int addTrailWhitespace = 150 - textLength;
	for(int i = 0; i < addTrailWhitespace; i++){
		sprintf((char*) (&progressText + textLength + i), "_"); //for debug
		// sprintf(&progressText + textLength + i, " ");
	}

	for(int i = 0; i < 150; i++) printf("\b");
    printf("%s", progressText);
}	


char* FileToString(char* path){
	FILE* buffer_txt;
	buffer_txt = fopen(path, "rb");
	char *returnString = 0;
	if(buffer_txt){
		fseek(buffer_txt, 0, SEEK_END);
		long length = ftell(buffer_txt);
  		fseek (buffer_txt, 0, SEEK_SET);
		returnString = malloc(length);
		if(returnString) {
			fread(returnString, 1, length, buffer_txt);
		}
		fclose(buffer_txt);
		return returnString;
	}
	return NULL;
}