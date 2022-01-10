#ifndef _SERVER_H_
#define _SERVER_H_

#include <arpa/inet.h> // htons()
#include <stdio.h>     // printf(), perror()
#include <stdlib.h>    // atoi()
#include <string>
#include <sys/socket.h> // socket(), bind(), listen(), accept(), send(), recv()
#include <unistd.h>     // close()

//#include "helpers.h" // make_server_sockaddr(), get_port_number()
using namespace std;

int handle_connection(int connectionfd);
int run_server(int port, int queue_size);
int handle_rb_conn(int connectionfd);
int handle_wb_conn(int connectionfd);

#endif
