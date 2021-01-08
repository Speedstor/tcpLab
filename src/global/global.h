#include <netdb.h>
#include <pthread.h>

#ifndef GLOBAL_VARIABLES
#define GLOBAL_VARIABLES
    pthread_t cycle_thread; //for repeatedly sending packages
    int cycle_running;
    int in_progress;
    int running;
    int ifMultithread;
    int verbose;
    int animation;
    int recordJson;

    struct addrinfo addr_settings; //global so that it need to only set once

    int recordDB;

    int handledCount;
#endif