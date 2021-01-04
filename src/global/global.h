#include <netdb.h>
#include <pthread.h>

#ifndef GLOBAL_VARIABLES
#define GLOBAL_VARIABLES
    pthread_t cycle_thread; //for repeatedly sending packages
    int cycle_running;
    int in_progress;
    int running;
    int ifMultithread;

    struct addrinfo addr_settings; //global so that it need to only set once

    int recordDB;
#endif