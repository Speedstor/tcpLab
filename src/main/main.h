#ifndef MAIN_HEADER
#define MAIN_HEADER
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <libgen.h>
    #include <errno.h>
    #include <string.h>
    #include <getopt.h>
    #include <sys/types.h>
    
    #include <sys/socket.h>

    #include <time.h> 
    #include <netdb.h>

    #include <pthread.h>

#include <arpa/inet.h>

    //self-defined
    #include "../common/definitions.h"
    #include "../common/control.h"
    #include "../socket/socketCore.h"
    #include "../socket/packetCore.h"
    #include "../common/spz.h"
    #include "../common/record.h"
    #include "../socket/serverThread.h"
    #include "../socket/tcp/tcp.h"

#endif
