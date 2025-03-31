#ifndef PORTS_H
#define PORTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hwt905.h"

#define PORT 8080  // Порт, на котором сервер будет принимать подключения


void form_answer_buffer(char* buffer, size_t size, hwt905_values *data, int count);
bool send_data( hwt905_values *data, int client_socket);
bool start_TCP_server(int *server_fd, struct sockaddr_in *address, int *opt, int *adrlen);



#endif // POTS_H
