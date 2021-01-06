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


    //self-defined
    #include "../common/definitions.h"
    #include "../common/control.h"
    #include "../socket/socketCore.h"
    #include "../common/spz.h"
    #include "../common/record.h"
    #include "../socket/receiveThread.h"
    #include "../socket/tcp/tcp.h"

    #include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#endif
