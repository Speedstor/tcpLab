#ifndef SPZ_H
#define SPZ_H
    int msleep(long msec);

    void refresh_progressBar();
    void print_progressBar(char* text, int percentage);
    void end_progressBar(int status);
    
    char* toBinaryString(void* target, int size);
    
    unsigned long long mTime();
    
    char* getTimestamp();
#endif

#ifndef STATUS_BUFFER_MAX
#define STATUS_BUFFER_MAX 1200
#endif

#ifndef STATUS_BUFFER
#define STATUS_BUFFER
    char* lastProgressText;
    char *statusBuffer[STATUS_BUFFER_MAX];
#endif
