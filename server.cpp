#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <bits/stdc++.h> 

int main(int argc, char **argv) {

    int ret;
    fd_set crowd_fds;
    fd_set tmp_fds;
    int fdmax;
    struct sockaddr_in tcp_addr;
    struct sockaddr_in udp_addr;
    struct sockaddr_in client_addr;
    socklen_t tcp_socket_len = sizeof(sockaddr);
    socklen_t udp_socket_len = sizeof(sockaddr);
    char buffer[BUFLEN];
    tcp_message tcp_msg;
    client new_client;

    // retine id-ul clientului si mesajele care ii corespund
    unordered_map<string, vector<tcp_message>> messages_topic_queue;
    // retine topicul si clientii abonati la el
    unordered_map<string, vector<string>> subscribers_topic_connected;
    // retine clientul si file descriptorul clientului
    unordered_map<string, int> client_to_socket;
    // retine file descriptorul si clientul
    unordered_map<int, client> clients;
    // id client la vector de pereche nume topic, sf
    unordered_map<string, vector<pair<string, int>>> abonamente_per_client;

    if (argc != 2) {
		usage(argv[0]);
	}

    // creare socketul TCP
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "Nu se poate crea socket-ul TCP!");

    // creare socketul UDP
    int udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "Nu se poate crea socket-ul UDP!");

    // verificare port
    int portno = atoi(argv[1]);
	DIE(portno < 1024, "Port incorect");

    // completare campuri socketi
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(portno);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(portno);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    FD_ZERO(&crowd_fds);

    ret = bind(tcp_socket, (sockaddr *) &tcp_addr, sizeof(sockaddr_in));
	DIE(ret < 0, "Nu se poate face bind cu socket-ul TCP\n");

    FD_SET(tcp_socket, &crowd_fds);

    ret = bind(udp_socket, (sockaddr *) &udp_addr, sizeof(sockaddr_in));
	DIE(ret < 0, "Nu se poate face bind cu socket-ul UDP\n");

    FD_SET(udp_socket, &crowd_fds);

    ret = listen(tcp_socket, MAX_CLIENTS);
	DIE(ret < 0, "Nu se poate face listen");

    FD_SET(0, &crowd_fds);
    if (udp_socket < tcp_socket) {
        fdmax = tcp_socket;
    } else {
        fdmax = udp_socket;
    }

    int ok = 0;

    // pana cand primeste comanda 'exit'
    while (ok == 0) {
        memset(buffer, 0, BUFLEN);
        tmp_fds = crowd_fds;

        ret = select(fdmax + 1, &tmp_fds, nullptr, nullptr, nullptr);
		DIE(ret < 0, "Nu se poate face select");

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == 0) {
                    // primire comanda stdin
                    fgets(buffer, BUFLEN - 1, stdin);

                    if (strcmp(buffer, "exit\n") == 0) {
                        ok = 1;

                        tcp_message exit_msg = {};
                        strcpy(exit_msg.type, "end");

                        // inchiderea socketilor clientilor activi
                        for (int j = 5; j <= fdmax; j++) {
                            if (FD_ISSET(j, &crowd_fds)) {
                                send(j, (char*) &exit_msg,
                                 sizeof(tcp_message), 0);
                                DIE(ret < 0, "Nu se poate da exit\n");
                            }
                        }

                        ret = shutdown(tcp_socket, SHUT_RDWR);
                        DIE(ret < 0, "Nu pot da exit TCP general\n");
                        break;

                    } else {
                        printf("Nu exista aceasta comanda!\n");
                    }
                } else if (i == tcp_socket) {
                    // a venit o cerere de conexiune pe socketul inactiv
                    // (cel cu listen), pe care serverul o accepta
                    int client_socket = accept(i, (sockaddr *)&client_addr,
                    &tcp_socket_len);
                    DIE(client_socket < 0, "Nu se poate da accept");

                    int value = 1;
                    ret = setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY,
                     (char *)&value, sizeof(int));
                    DIE(ret < 0, "Nu s-a putut dezactiva algoritmul lui Neagle");

                    FD_SET(client_socket, &crowd_fds);
                    if (fdmax < client_socket) {
                        fdmax = client_socket;
                    }

                    // primire informatii
                    ret = recv(client_socket, buffer, BUFLEN - 1, 0);
                    DIE(ret < 0, "Nu s-a primit nimic de la clientul TCP!");

                    strcpy(new_client.id, buffer);

                    // daca nu l-a gasit
                    if (client_to_socket.find(new_client.id) == client_to_socket.end()) {
                        clients.insert({client_socket, new_client});

                        // notificare la conectarea unui nou client
                        std::string id(new_client.id);
                        client_to_socket[id] = client_socket;
                        printf("New client %s connected from %s:%hu.\n",
                        new_client.id, inet_ntoa(client_addr.sin_addr),
                        ntohs(client_addr.sin_port));
                    } else {
                        // s-a reconectat un vechi client
                        std::string id(new_client.id);
                        client_to_socket[id] = client_socket;

                        printf("New client %s connected from %s:%hu.\n",
                        new_client.id, inet_ntoa(client_addr.sin_addr),
                        ntohs(client_addr.sin_port));

                        // se trimit toate mesajele pe care le-a pierdut
                        for (auto message : messages_topic_queue[id]) {
                             ret = send(client_socket, 
                            (char*) &message, sizeof(tcp_message), 0);
                            DIE(ret < 0, "Nu se poate trimite mesajul\n");
                        }

                        // se goleste coada de mesaje
                        messages_topic_queue[id].clear();
                    }
 
                } else if (i == udp_socket) {
                    // primire mesaj de la un client UDP
                    struct recv_UDP_message *udp_msg = 
                    (struct recv_UDP_message *)malloc(sizeof(recv_UDP_message));

                    ret = recvfrom(udp_socket, udp_msg,
                    sizeof(recv_UDP_message), 0,
                    (sockaddr*) &udp_addr, &udp_socket_len);
                    DIE(ret < 0, "Nu s-a primit nimic de la clientul UDP!\n");

                    strcpy(tcp_msg.ip, inet_ntoa(udp_addr.sin_addr));
                    tcp_msg.port = ntohs(udp_addr.sin_port);
                    convert_msg(udp_msg, &tcp_msg);
                    
                    std::string client_topic(tcp_msg.topic);
                    for (std::string subscriber :
                     subscribers_topic_connected[client_topic]) {
                        int client_socket = client_to_socket[subscriber];
                        // daca clientul e inactiv
                        if (client_socket < 0) {
                            // daca e in lista de abonati si are sf = 1
                                for (auto client : 
                                abonamente_per_client[subscriber]) {
                                    if (client.first == client_topic &&
                                     client.second == 1) {
                                            messages_topic_queue[subscriber].
                                            push_back(tcp_msg);
                                    }
                                }
                        } else if (client_socket >= 5) {
                            ret = send(client_socket, 
                            (char*) &tcp_msg, sizeof(tcp_message), 0);
                            DIE(ret < 0,
                            "Nu se poate trimite mesajul la clientul TCP\n");
                        }
                    }

                } else {
                    // primire comanda de la un client TCP

                    ret = recv(i, buffer, BUFLEN - 1, 0);
                    DIE(ret < 0, "Nu s-a primit nimic de la clientul TCP\n");

                    if (ret > 0) {
                        server_message *message = 
                        (server_message *)malloc(sizeof(server_message));
			            memcpy(message, &buffer, sizeof(server_message));
                        std::string id_client(message->id); 
                        std::string client_topic(message->topic);

                        // subscribe
                        if (message->command == 1) {
                            std::vector<int>::iterator iterator;
                            // nu exista cheia in mapa
                            if (subscribers_topic_connected.find(client_topic)
                             == subscribers_topic_connected.end()) {
                                std::vector<string> empty_subscriebers;
                                subscribers_topic_connected[client_topic]
                                 = empty_subscriebers;   
                            }

                            subscribers_topic_connected[client_topic].
                            push_back(id_client);


                            abonamente_per_client[message->id].
                            push_back(make_pair(message->topic, message->sf));
                            
                        // unsubscribe
                        } else if (message->command == 0) {

                            std::vector<string> clients_after_unsubscription;
                            for (std::string subscriber : 
                            subscribers_topic_connected[client_topic]) {
                                // pastrez clientii care au id diferit de cel
                                // primit pe cererea de unsubscribe
                                if (id_client != subscriber) {
                                    clients_after_unsubscription.
                                    push_back(subscriber);
                                }
                            }
                            subscribers_topic_connected[client_topic]
                             = clients_after_unsubscription;
                        }

                    } else if (ret == 0) {
                        printf("Client %s disconnected.\n",
                        clients[i].id);
                        client_to_socket[clients[i].id] = -10;
                        close(i);
                        FD_CLR(i, &crowd_fds);

                        // se recalculeaza fdmax
                        int k = fdmax;
                        while (k > 2) {
                            if (FD_ISSET(k, &crowd_fds)) {
                                fdmax = k;
                                break;
                            }
                            k--;
                        }
                    }
                }
            }
        }
    }
}