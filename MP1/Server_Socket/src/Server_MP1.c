#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include "Server_MP1.h"
#include "Server_sig_handle.h"
#include "common.h"

int main(int argc, char* argv[])
{
    // Declare 2 File Descriptor used for Listen and Connect
    int listenfd, connfd;

    char buff[MAXLINE];
    int recv_bytes;
    struct sockaddr_in serv_addr;
    char ipaddr[64];
    char ipaddr_client[64];
    int port;
    int port_client;

    // Get port number from the only input argument
    if(argc != 2) {
        printf("Incorrect number of parameters!\n");
        printf("Usage example:\n");
        printf("./Server <Port Number>");
        exit(EXIT_FAILURE);
    }
    else{
        port = atoi(argv[1]);
    }

    // Initialize signal sets
    int state;
    struct sigaction sig;
    // state = handle_child_proc(&sig);

    // Create Socket
    if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("[Error]Socket -- ");
        exit(EXIT_FAILURE);
    }

    // Initialize Socket Address Information
    // Defined in <netinet/in.h>
    memset(&serv_addr, 0, sizeof(serv_addr)); 

    // Configure Socket Address
    serv_addr.sin_family = AF_INET;                     //Address Family: Ipv4
    serv_addr.sin_port = htons(port);                   //Configure Port Number
    //serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);   //Configure IP Address
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);      //Configure IP Address
    strcpy(ipaddr, inet_ntoa(serv_addr.sin_addr));
    printf("***********************************\n");
    printf("    Server IP Address: %s\n", ipaddr);
    printf("***********************************\n\n");

    // Binding
    if(bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        perror("[Error]Binding -- ");
        exit(EXIT_FAILURE);
    }

    // Listening-Queue: 5
    if(listen(listenfd, 5) < 0){
        perror("[Error]Listening -- ");
        exit(EXIT_FAILURE);
    }

    // Connect/Accpet
    // char Conn_Inform[] = "Connect Server Successfully\n";
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    while(1){
        if((connfd = accept(listenfd, (struct sockaddr*) &clnt_addr, &clnt_addr_size)) < 0){
            //Deal with Interrupt System Call
            if(errno == EINTR)
                continue;
            else
                perror("[Error]Accept -- ");
                exit(EXIT_FAILURE);
        }
        // Print IP address&Ports connected clients
        inet_ntop(AF_INET, (struct sockaddr*) &clnt_addr.sin_addr.s_addr, ipaddr_client, sizeof(ipaddr_client));
        port_client = ntohs(clnt_addr.sin_port);
        printf("[Client connected] IP Address: %s ; Port: %d ; \n",
                ipaddr_client, port_client );

        // Create multithreads to enable multiple clients connections
        pid_t pid = fork();
        // Child process
        if(pid == 0){
            printf("[Child process created] PID = %d ;\n\n", getpid());
            // Listenning is not necessary for child processes
            close(listenfd);
            // Regular Service
            while(1){
                memset(buff, 0, MAXLINE);
                recv_bytes = read(connfd, buff, MAXLINE);
                if(recv_bytes == 0){
                    printf("[%s:%d]: Connection Terminated\n", ipaddr_client, port_client);
                    close(connfd);
                    // break;
                    return 0;
                }
                else if(recv_bytes > 0){
                    printf("Message From [%s:%d]:\n", ipaddr_client, port_client);
                    read_newline(buff);
                    if(write(connfd, buff, recv_bytes) < 0){
                        perror("[Error]Send -- ");
                        exit(EXIT_FAILURE);
                    }
                }
                else if(recv_bytes < 0){
                    perror("[Error]Read -- ");
                    close(connfd);
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if(pid > 0){
            close(connfd);
            // Detect SIGCHLD when clients are closed, and then kill corresponding child processes
            state = handle_child_proc(&sig);
        }
        else if(pid < 0){
            printf("[Error] Fail to create a new process \n ");
            close(connfd);
        }
    }

    close(listenfd);

    return 0;
}
