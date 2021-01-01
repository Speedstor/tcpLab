#include <stdio.h>
#include <string.h>

#include <malloc.h>
#include <errno.h>
#include <time.h> 

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


//https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec) {
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