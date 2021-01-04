#ifndef SPZ_COMMON_C_FUNC
#define SPZ_COMMON_C_FUNC
    char* FileToString(char* path);
    int msleep(long msec);
    void progressBar_print(char* text, int percentage);
    char* toBinaryString(void* target, int size);
    char* getTimestamp();
#endif
