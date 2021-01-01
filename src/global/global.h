#include <pthread.h>

#ifndef GLOBAL_VARIABLES
#define GLOBAL_VARIABLES
    pthread_t cycle_thread; //for repeatedly sending packages
    int cycle_running;
    int in_progress;
    int running;

    int send_socket;
    int receive_socket;
#endif