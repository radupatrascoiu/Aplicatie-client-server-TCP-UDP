#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <netinet/tcp.h>
#include <unordered_map>
#include <zconf.h>
#include <vector>
using namespace std;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// mesaj transmis la un client TCP
struct tcp_message {
    char ip[16];
    uint16_t port;
    char topic[50];
    char type[10];
    char content[1500];
};

// mesaj transmis de un client TCP la server
struct server_message {
    int command;
    char topic[50];
    int sf;
    char id[10];
};

// mesaj primit de la un client UDP
struct recv_UDP_message {
    char topic[50];
    uint8_t type;
    char content[1500];
};

struct client {
    char id[11];
};

#define BUFLEN	    1600 // dimensiunea maxima a calupului de date
#define MAX_CLIENTS	500	// numarul maxim de clienti in asteptare

void convert_msg(recv_UDP_message* udp_msg, tcp_message* tcp_msg);
void usage(char *file);

#endif
