#include <stdio.h>
#include <string.h>
#include <malloc.h>

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