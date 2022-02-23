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
#include "common.h"
#include "sbcp_protocol.h"
#include "Server.h"
#include "Server_sbcp.h"
#include "Server_sig_handle.h"

int main(int argc, char* argv[])
{
    // File Descriptors used for Listen and Connect
    int listenfd, connfd;
    int max_fd;      // Maximum number of file decriptors for all clients
    fd_set read_set; // For checking
    fd_set all_set;  // For Add/Remove


    char buff[MAXLINE];
    int recv_bytes;


    char ipaddr[64];        // Server IP address
    char ipaddr_client[64]; // Client IP address
    int port;               // Server port
    int port_client;        // Client port
    int max_client;         // Maximum number of clients
    int curr_client = 0;    // Current number of clients
    int ipv4;

    // Get port number from the only input argument
    if(argc != 4) {
        printf("Incorrect number of parameters!\n");
        printf("Usage example:\n");
        printf("./Server <IP Address> <Port Number> <Maximum number of clients>\n");
        exit(EXIT_FAILURE);
    }
    else{
        port = atoi(argv[2]);
        max_client = atoi(argv[3]);
    }

    // Identify the protocol of the input IP address
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_IP;

    int ipaddr_chk = getaddrinfo(argv[1], NULL, &hints, &res);
    if(ipaddr_chk != 0){
        printf("[Error] Cannot parse the input IP address!\n");
        exit(EXIT_FAILURE);
    }
    else{
        if(res->ai_family == AF_INET){
            ipv4 = 1;
        }
        else if(res->ai_family == AF_INET6)
            ipv4 = 0;
        else
            printf("[DEBUG] Inindentified IP address.");
    }

    freeaddrinfo(res);


    // Configure Socket Address
    if(ipv4){
        struct sockaddr_in serv_addr;
        // Initialize Socket Address Information
        memset(&serv_addr, 0, sizeof(serv_addr)); 

        // Create Socket
        if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
            perror("[Error]Socket -- ");
            exit(EXIT_FAILURE);
        }

        serv_addr.sin_family = AF_INET;                     //Address Family: Ipv4
        serv_addr.sin_port = htons(port);                   //Configure Port Number
        //serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);   //Configure IP Address
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);      //Configure IP Address
        strcpy(ipaddr, inet_ntoa(serv_addr.sin_addr));

        // Binding
        if(bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
            perror("[Error]Binding -- ");
            exit(EXIT_FAILURE);
        }
    }
    else{
        struct sockaddr_in6 serv_addr;
        // Initialize Socket Address Information
        memset(&serv_addr, 0, sizeof(serv_addr)); 

        // Create Socket
        if((listenfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0){
            perror("[Error]Socket -- ");
            exit(EXIT_FAILURE);
        }

        serv_addr.sin6_family = AF_INET6;                     //Address Family: Ipv4
        serv_addr.sin6_port = htons(port);                   //Configure Port Number
        //serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);   //Configure IP Address
        serv_addr.sin6_addr = in6addr_any;      //Configure IP Address
        char in6_addr[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(serv_addr.sin6_addr), in6_addr, INET_ADDRSTRLEN);
        strcpy(ipaddr, in6_addr);

        // Binding
        if(bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
            perror("[Error]Binding -- ");
            exit(EXIT_FAILURE);
        }
    }

    printf("***********************************\n");
    printf("           SBCP CHAT ROOM          \n");
    printf("    Server IP Address: %s\n", ipaddr);
    printf("    Server Port Number: %d\n", port);
    printf("    Maximum Client Numbers: %d\n", max_client);
    printf("***********************************\n\n");


    // Listening-Queue: 5
    if(listen(listenfd, 5) < 0){
        perror("[Error]Listening -- ");
        exit(EXIT_FAILURE);
    }
    // Connect/Accpet
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    FD_ZERO(&all_set);
    FD_ZERO(&read_set);
    FD_SET(listenfd, &all_set); // Add FD for listening to the set for reading

    max_fd = listenfd;

    // Create a database to store information of users
    struct user* user_db = create_user_db(max_client + 10);

    while(1){

        read_set = all_set;
        //printf("[DEBUG] Select Wait...\n");

        // arg[1]: Check if fds in read_set are ready for reading
        // arg[2]: No set for writing needed
        // arg[3]: No set for exception needed
        // arg[4]: No set for timeout needed. Keep blocking until any fd is ready or error
        if(select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0){
            perror("[Error]Select -- ");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i <= max_fd; i++){
            //Check new data for reading
            if(FD_ISSET(i, &read_set)){
//****************************************************
//            New client requests to connect
//****************************************************
                if(i == listenfd){
                    // printf("[DEBUG]NEW FD: %d\n", i);
                    if((connfd = accept(listenfd, (struct sockaddr*) &clnt_addr, &clnt_addr_size)) < 0){
                        perror("[Error]Accept -- ");
                        exit(EXIT_FAILURE);
                    }
                    else{
                        int check_flag = sbcp_JOIN_server(listenfd, connfd, &read_set, buff, &curr_client, max_client, max_fd, user_db);
                        if(check_flag < 0){
                            sbcp_NAK(connfd, check_flag);
                            close(connfd);
                        }
                        else{
                            FD_SET(connfd, &all_set);
                            sbcp_ACK(listenfd, connfd, max_fd, curr_client, user_db, &all_set);
                            sbcp_ONLINE(listenfd, connfd, max_fd, user_db, &all_set);
                            if(connfd > max_fd){
                                max_fd = connfd;
                            }
                            // Print IP address&Ports connected clients
                            if(ipv4){
                                inet_ntop(AF_INET, (struct sockaddr*) &clnt_addr.sin_addr.s_addr, ipaddr_client, sizeof(ipaddr_client));
                                port_client = ntohs(clnt_addr.sin_port);
                                printf("[NEW USER ADDED] IP Address: %s ; Port: %d ; \n", ipaddr_client, port_client );
                            }
                            printf("**************************************************\n\n");
                        }
                    }
                }
//****************************************************
//        Communicate with existed clients
//****************************************************
                else{
                    // printf("[DEBUG] FD EXISTS: %d\n", i);
                    memset(buff, 0, MAXLINE);
                    recv_bytes = read(i, buff, MAXLINE);

                    if(recv_bytes == 0){
                        sbcp_OFFLINE(listenfd, i, max_fd, user_db, &all_set);
                        remove_user_db(user_db, &curr_client, i, max_fd);
                        // printf("[%s:%d]: Connection Terminated\n", ipaddr_client, port_client);
                        FD_CLR(i, &all_set);
                        close(i);
                        // return 0;
                    }
                    else if(recv_bytes > 0){
                        // printf("Message From [%s:%d]:\n", ipaddr_client, port_client);
                        // sbcp_print_dbg_header(buff);
                        // sbcp_print_dbg_post_pack(buff);

                        char* send_fwd_msg = (char*)malloc(sizeof(char) * 512);
                        int recv_type = sbcp_SEND_server(buff, send_fwd_msg);
                        if(recv_type < -1){
                            printf("[INVALID MESSAGE] From: %s\n", user_db[i].name);
                        }
                        else if(recv_type == -1){
                            printf("User [ %s ] is IDLE.\n", user_db[i].name);
                            sbcp_IDLE(listenfd, i, max_fd, user_db, &all_set);
                        }
                        else{
                            printf("| %s | ", user_db[i].name);
                            read_newline(send_fwd_msg);
                            sbcp_FWD(user_db[i].name, send_fwd_msg, listenfd, i, max_fd, &all_set);
                        }
                        free(send_fwd_msg);
                        /*
                        if(write(i, buff, recv_bytes) < 0){
                            perror("[Error]Send -- ");
                            exit(EXIT_FAILURE);
                        }
                        */
                    }
                    else if(recv_bytes < 0){
                        perror("[Error]Read -- ");
                        sbcp_OFFLINE(listenfd, i, max_fd, user_db, &all_set);
                        remove_user_db(user_db, &curr_client, i, max_fd);
                        FD_CLR(i, &all_set);
                        close(i);
                        exit(EXIT_FAILURE);
                    }
                } // else: i != listenfd
            } // if: Check read_set
        } // For-Loop: iterate read_set
    } // while(1)

    close(listenfd);

    return 0;
}
