#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <bits/stdc++.h>
using namespace std;

int main(int argc, char **argv)
{
	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc != 4) {
		usage(argv[0]);
	}

	fd_set crowd_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea crowd_fds

	FD_ZERO(&tmp_fds);
	FD_ZERO(&crowd_fds);

	// creare socket pentru comunicarea cu serverul
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Nu se poate deschide socketul!");

	// setare file descriptori
	FD_SET(sockfd, &crowd_fds);
	FD_SET(0, &crowd_fds);
	fdmax = sockfd;

	// completare informatii pentru pentru socketul socketul TCP
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "Adresa IP incorecta!");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "Eroare la conectarea cu serverul");

	ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
	DIE(ret < 0, "Nu se poate trimite catre server");

	int value = 1;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&value,
	sizeof(int));
	DIE(ret < 0, "Nu s-a putut dezactiva algoritmul lui Neagle");

	while (1) {
  		// se citeste de la tastatura
		tmp_fds = crowd_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Eroare la selectare socket");

		// mesaj de la stdin
		if (FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			buffer[strlen(buffer) - 1] = 0;

			if (strcmp(buffer, "exit") == 0) {
				ret = shutdown(sockfd, SHUT_RDWR);
				DIE(ret < 0, "Nu se poate da exit\n");
				break;
			} else {

				server_message message;
				const char delimiter[2] = " ";
				char *token = strtok(buffer, delimiter);

				strcpy(message.id, argv[1]);

				if (strcmp(token, "subscribe") == 0) {
					message.command = 1;
					token = strtok(nullptr, delimiter);
					strcpy(message.topic, token);
					
					token = strtok(nullptr, delimiter);
					char *sf = strdup(token);
					message.sf = atoi(sf);

				} else if (strcmp(token, "unsubscribe") == 0) {
					message.command = 0;
					token = strtok(NULL, delimiter);
					strcpy(message.topic, token);

					printf("unsubscribe %s\n\n", message.topic);

				} else {
					printf("Comanda invalida\n");
				}

				// se trimite mesaj la server
				ret = send(sockfd, (char*) &message, sizeof(message), 0);
				DIE(ret < 0, "Nu s-a putut trimite la server");
			
				if (message.command == 1) {
					printf("subscribed %s\n", message.topic);
				} else {
					printf("unsubscribed %s\n", message.topic);
				}
			}

			// mesaj de la un server
		} else if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, (sizeof(tcp_message) + 1));

			ret = recv(sockfd, buffer, sizeof(tcp_message), 0);
			DIE(ret < 0, "Nu s-a primit nimic de la server");

			tcp_message* message = (tcp_message *)malloc(sizeof(tcp_message));
			memcpy(message, &buffer, sizeof(tcp_message));

			// inchidere
			if (strncmp(message->type, "end", 3) == 0) {
				ret = shutdown(sockfd, SHUT_RDWR);
			    DIE(ret < 0, "Nu pot da exit\n");
				exit(EXIT_SUCCESS);
			} else {
				// afisare informatii primite
				printf("%s:%hu - %s - %s - %s\n", message->ip, message->port,
				message->topic, message->type, message->content);
			}
		}
	}

	close(sockfd);

	return 0;
}
