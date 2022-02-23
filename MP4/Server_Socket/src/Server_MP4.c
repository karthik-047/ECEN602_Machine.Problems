#include "Server.h"

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
    int ipv4;

    // Get port number from the only input argument
    if(argc != 3) {
        printf("Incorrect number of parameters!\n");
        printf("Usage example:\n");
        printf("./Proxy <IP Address> <Port Number>\n");
        exit(EXIT_FAILURE);
    }
    else{
        port = atoi(argv[2]);
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
            printf("[Error] Inindentified IP address.");
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
        serv_addr.sin6_port = htons(port);                    //Configure Port Number
        //serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);     //Configure IP Address
        serv_addr.sin6_addr = in6addr_any;                    //Configure IP Address
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
    printf("       HTTP PROXY SERVER START     \n");
    printf("    Server IP Address: %s\n", ipaddr);
    printf("    Server Port Number: %d\n", port);
    printf("***********************************\n\n");


    // Listening-Queue: 5
    if(listen(listenfd, 5) < 0){
        perror("[Error]Listening -- ");
        exit(EXIT_FAILURE);
    }

    // Initialize the cache of current proxy server
    http_cache_t* proxy_cache = cache_init();

    // Connect/Accpet
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    FD_ZERO(&all_set);
    FD_ZERO(&read_set);
    FD_SET(listenfd, &all_set); // Add FD for listening to the set for reading

    max_fd = listenfd;

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
// ****************************************************
//            New client requests to connect
// ****************************************************
                if(i == listenfd){
                    // printf("[DEBUG]NEW FD: %d\n", i);
                    if((connfd = accept(listenfd, (struct sockaddr*) &clnt_addr, &clnt_addr_size)) < 0){
                        perror("[Error]Accept -- ");
                        exit(EXIT_FAILURE);
                    }
                    else{
                            FD_SET(connfd, &all_set);
                            if(connfd > max_fd){
                                max_fd = connfd;
                            }
                            // Print IP address&Ports connected clients
                            if(ipv4){
                                inet_ntop(AF_INET, (struct sockaddr*) &clnt_addr.sin_addr.s_addr, ipaddr_client, sizeof(ipaddr_client));
                                port_client = ntohs(clnt_addr.sin_port);
                                printf("\n[NEW USER ADDED] IP Address: %s ; Port: %d ; \n\n", ipaddr_client, port_client );
                            }
                        }
                }
// ****************************************************
//        Communicate with existed clients
// ****************************************************
                else{
                    // printf("[DEBUG] FD EXISTS: %d\n", i);
                    memset(buff, 0, MAXLINE);
                    recv_bytes = read(i, buff, MAXLINE);

                    if(recv_bytes == 0){

                        FD_CLR(i, &all_set);
                        close(i);
                        // return 0;
                    }
                    else if(recv_bytes > 0){
                        // 1. Get request from the client
                        http_req_t req_proxy;
                        if(http_req_parse(buff, &req_proxy) < 0){
                            printf("[Error] Parsing request failed...\n");
                        }
                        printf("[INFO] Request message from client: \n%s", buff);
                        // Get current time
                        time_t currtime;
                        struct tm* currtime_tm = (struct tm*)malloc(sizeof(struct tm));
                        char currtime_str[TIME_L];
                        time(&currtime);
                        currtime_tm = gmtime(&currtime);
                        time_transform_tm2str(currtime_tm, currtime_str);

                        // 2. Check cache entries for the request
                        int req_cache_index = cache_access_mod(req_proxy, proxy_cache);
                        // Cache miss
                        if(req_cache_index < 0){
                            printf("[INFO] Cache miss\n");
                            // Connect remote server for the updated page
                            int serv_fd = connect_server(req_proxy);
                            if(serv_fd < 0){
                                printf("[Error] Connect server failed...\n");
                                exit(EXIT_FAILURE);
                            }
                            // Send GET request to the remote server
                            if(send_http_get(serv_fd, buff, req_proxy) < 0){
                                printf("[Error] Send request failed...\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("[INFO] Send requests to the remote serveri:\n");
                            printf("%s", buff);

                            // Receive the file and cache it
                            char res_header[HEADER_L] = {0};
                            char recv_file[FILENAME_L + HOSTNAME_L] = "./Cache/";
                            char newname[FILENAME_L];
                            strcpy(newname, req_proxy.file);
                            name_mod(newname);
                            strcat(recv_file, req_proxy.host);
                            strcat(recv_file, newname);
                            printf("[INFO] Create the cache file: %s\n", recv_file);
                            if(recv_http_get(serv_fd, buff, res_header, recv_file) < 0){
                                printf("[Error] Receive data failed...\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("[INFO] Response header:\n%s\n", res_header);
                            int new_index;
                            new_index = cache_new_page(res_header, req_proxy, recv_file, proxy_cache);
                            // Send the file to the client
                            if(http_respond_client(proxy_cache, new_index, recv_file, i, buff) < 0){
                                printf("[Error] Failed to respond.\n");
                                exit(EXIT_FAILURE);
                            }

                            close(serv_fd);

                        }
                        // Cache hit
                        else{
                            printf("[INFO] Cache hit\n");
                            // Connect remote server for the updated page
                            int serv_hit_fd = connect_server(req_proxy);
                            if(serv_hit_fd < 0){
                                printf("[Error] Connect server failed...\n");
                                exit(EXIT_FAILURE);
                            }
                            // Send GET request to the remote server
                            if(send_http_get_hit(serv_hit_fd, buff, proxy_cache, req_cache_index) < 0){
                                printf("[Error] Send request failed...\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("[INFO] Send requests to the remote serveri:\n");
                            printf("%s", buff);

                            cache_lru_hit(proxy_cache, req_cache_index);

                            char res_header_hit[HEADER_L] = {0};
                            int res_recv = recv_http_get_hit(serv_hit_fd, buff, res_header_hit, proxy_cache[req_cache_index].filepath);
                            printf("[INFO] Response header:\n%s\n", res_header_hit);
                            if(res_recv < 0){
                                printf("[Error] Receive data failed...\n");
                                exit(EXIT_FAILURE);
                            }
                            if(res_recv >= 0){
                                // Send the file to the client
                                if(http_respond_client(proxy_cache, req_cache_index, NULL, i, buff) < 0){
                                    printf("[Error] Failed to respond.\n");
                                    exit(EXIT_FAILURE);
                                }
                            }

                        }

                        FD_CLR(i, &all_set);
                        close(i);

                    }
                    else if(recv_bytes < 0){
                        perror("[Error]Read -- ");
                        FD_CLR(i, &all_set);
                        close(i);
                        exit(EXIT_FAILURE);
                    }
                } // else: i != listenfd
            } // if: Check read_set
        } // For-Loop: iterate read_set
    } // while(1)

    free(proxy_cache);
    close(listenfd);

    return 0;
}
