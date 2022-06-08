#include "Server.h"
#include "Server_sig_handle.h"
#include "Server_tftp.h"
#include "common.h"

int main(int argc, char* argv[])
{
    // Declare 2 File Descriptor used for Listen and Connect
    int listenfd, connfd;

    //buffers for send and receive
	unsigned char buff[MAXLINE];
    unsigned char buff_send[BLOCKSIZE + 4];
    int recv_bytes;
	
	//socket and server attributes
    struct sockaddr_in serv_addr;
    char ipaddr[64];
    char ipaddr_client[64];
    int port;
    int port_client;

    // Get port number from the only input argument
    if(argc != 2) {
        printf("Incorrect number of parameters!\n");
        printf("Usage example:\n");
        printf("./Server <Port Number>\n");
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
    if((listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("[Error]Socket -- ");
        exit(EXIT_FAILURE);
    }

    // Initialize Socket Address Information
    // Defined in <netinet/in.h>
    memset(&serv_addr, 0, sizeof(serv_addr)); 

    // Configure Socket Address
    serv_addr.sin_family = AF_INET;                     //Address Family: Ipv4
    serv_addr.sin_port = htons(port);                   //Configure Port Number
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);      //Configure IP Address
    strcpy(ipaddr, inet_ntoa(serv_addr.sin_addr));
    printf("***********************************\n");
    printf("          TFTP SERVER START        \n");
    printf("    Server IP Address: %s \n", ipaddr);
    printf("    Server Port: %d \n", port);
    printf("***********************************\n\n");

    // Binding
    if(bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        perror("[Error]Binding -- ");
        exit(EXIT_FAILURE);
    }
	
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    while(1){
        memset(buff, 0, MAXLINE);
        if((recv_bytes = recvfrom(listenfd, buff, sizeof(buff), 0, (struct sockaddr*) &clnt_addr, &clnt_addr_size)) < 0){
            //Deal with Interrupt System Call
            if(errno == EINTR)
                continue;
            else
                perror("[Error]Recvfrom -- ");
                exit(EXIT_FAILURE);
        }
        // Print IP address&Ports connected clients
        inet_ntop(AF_INET, (struct sockaddr*) &clnt_addr.sin_addr.s_addr, ipaddr_client, sizeof(ipaddr_client));
        port_client = ntohs(clnt_addr.sin_port);
        printf("[Client connected] IP Address: %s ; Port: %d ; \n",
                ipaddr_client, port_client );

        // Check tftp request in the initial access
        tftp_req_msg_t req_init;
        int req_flag = check_req(buff, &req_init);
        // Request failure for standard error types
        if(req_flag > 0){
            tftp_error_msg_t err_init_1;
            tftp_gen_error_msg(req_flag, "", &err_init_1);
            memset(buff, 0, MAXLINE);
            tftp_pack_error_msg(buff, err_init_1);
            sendto(listenfd, buff, sizeof(buff), 0, (struct sockaddr*) &clnt_addr, clnt_addr_size);
            continue;
        }
        // TODO: For not defined error type. Add specific error messages.
        else if(req_flag == 0){
            printf("[DEBUG] Fault not defined!\n");
            continue;
        }
        else{
            // Create multiprocesses to start new transfer
            pid_t pid = fork();
            // Child process
            if(pid == 0){
                // printf("[Child process created] PID = %d ;\n\n", getpid());
                close(listenfd);
                // Service
                if(tftp_transfer(req_init, buff, clnt_addr, clnt_addr_size, serv_addr) < 0){
                    printf("[DEBUG] Transfer error!\n");
                }
            }
            else if(pid > 0){
                // Detect SIGCHLD when clients are closed, and then kill corresponding child processes
                state = handle_child_proc(&sig);
            }
            else if(pid < 0){
                printf("[Error] Fail to create a new process \n ");
            }
        }
    }

    close(listenfd);

    return 0;
}
