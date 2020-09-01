#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

void usage(char *file) {
	fprintf(stderr, "Usage: %s ./subscriber <ID_Client> <IP_Server> <Port_Server>\n", file);
	exit(0);
}