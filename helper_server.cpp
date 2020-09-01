#include "helpers.h"
using namespace std;

#include <cstdint>
#include <string>
#include <cstring>
#include <unordered_map>
#include <zconf.h>
#include <vector>
#include <netinet/in.h>
#include <cmath>

void usage(char *file) {
	fprintf(stderr, "Usage: %s ./server <PORT_DORIT>\n", file);
	exit(0);
}

void convert_msg(recv_UDP_message* udp_msg, tcp_message* tcp_msg) {
    memcpy(&tcp_msg->topic, &udp_msg->topic, strlen(udp_msg->topic)+1);

    DIE(udp_msg->type > 3, "Type incorect!");

    if (udp_msg->type == 0) {
        strcpy(tcp_msg->type, "INT");
        unsigned char sign_bit;
        memcpy(&sign_bit, udp_msg->content, 1);
        DIE(sign_bit > 1, "Bit de semn incorect");

        int converted_number, nr;
        memcpy(&converted_number, udp_msg->content + 1, 4);
        
        if (sign_bit == 1) {
            nr = -ntohl(converted_number);
        } else {
            nr = ntohl(converted_number);
        }

        sprintf(tcp_msg->content, "%d", nr);

    } else if (udp_msg->type == 1) {
        strcpy(tcp_msg->type, "SHORT_REAL");
        double converted_number = ntohs(*(uint16_t*)(udp_msg->content));
        sprintf(tcp_msg->content, "%.2f", converted_number / 100);

    } else if (udp_msg->type == 2) {
        strcpy(tcp_msg->type, "FLOAT");

        unsigned char sign_bit;
        memcpy(&sign_bit, udp_msg->content, 1);
        DIE(sign_bit > 1, "Bit de semn incorect");

        float converted_number = ntohl(*(uint32_t*)(udp_msg->content + 1));
        converted_number /= pow(10, udp_msg->content[5]);
        
        if (sign_bit == 1) {
            converted_number = -converted_number;
        }

        sprintf(tcp_msg->content, "%lf", converted_number);

    } else if (udp_msg->type == 3) {
        strcpy(tcp_msg->type, "STRING");
        memcpy(tcp_msg->content, udp_msg->content, strlen(udp_msg->content)+1);
        tcp_msg->content[strlen(tcp_msg->content)] = '\0';

        
    }

}
