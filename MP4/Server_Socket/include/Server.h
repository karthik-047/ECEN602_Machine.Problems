#ifndef SERVER
#define SERVER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "Server_http.h"

#define MAXLINE 2048
#define Port    10000
#define SERV_IP "127.0.0.1"

#endif
