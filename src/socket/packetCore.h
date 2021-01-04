#include "../common/definitions.h"

#ifndef PACKET_CORE_HEADER
#define PACKET_CORE_HEADER
    int listenForPacket(Rsock_packet* recv_packet, int socket, int protocol, char dest_ip[IPV4STR_MAX_LEN], int dest_port, char src_ip[IPV4STR_MAX_LEN], int src_port, int ack_seq);
#endif