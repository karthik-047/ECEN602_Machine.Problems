#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <netdb.h>
#include "common.h"
#include "sbcp_protocol.h"
#define MAXLINE 1024

//Function to convert user data to SBCP format
char* join_server(char *username,int msg_type,int attribute_type)
{
   char* buff_join;
   struct sbcp_message *join_msg, jmsg;
   struct sbcp_attribute join_attr;
   join_msg = &jmsg;
   jmsg.msg_attribute = &join_attr;
   jmsg.vrsn = SBCP_VRSN;
   jmsg.type =  msg_type ;
   join_attr.payload = malloc(sizeof(char) * 512);
   strcpy(join_attr.payload,username); 
   join_attr.type= attribute_type ;
   join_attr.length=4+(int)strlen(username);
   jmsg.length = 4 + join_attr.length;
   
   buff_join = sbcp_pack(&jmsg);
   return buff_join;
}

int main(int argc, char *argv[])
{ 
	//checking number of arguments
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <username> <serverip> <port> \n", argv[0]);
        exit(1);
    }

    int clientfd;
    uint16_t portnumber;
    char username[20], ipaddr[50];
    char buff[MAXLINE], buffr[MAXLINE];
    char *buffs;
    int length=0, timeout_value=10;
    int send_bytes=0;
    int select_value = 0;
    int ipv4;
	int status;
	//File descriptors for select()
    fd_set masterfds, readfds;
	
    //setting timeout
    struct timeval tc;
    tc.tv_sec = timeout_value;

    //Assigning connection parameters
    portnumber = atoi(argv[3]);         
    strcpy(username,argv[1]);
    strcpy(ipaddr,argv[2]);
    
	struct addrinfo hints; 							//points to addrinfo filled with information below
	struct addrinfo *p=NULL,*servinfo=NULL;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC; 					//don't care about family
    hints.ai_socktype = SOCK_STREAM;				//Stream socket
    hints.ai_protocol = IPPROTO_TCP;				//TCP
	status = getaddrinfo(ipaddr,argv[3], &hints, &servinfo);
        if(status!=0)
        {
            printf("Error");
        }
		
		//Set addressing to IPv4/IPv6
		else{
        if(servinfo->ai_family == AF_INET){
            ipv4 = 1;
        }
        else if(servinfo->ai_family == AF_INET6)
            ipv4 = 0;
        else
		{
            perror("Unindentified IP address.");
			exit(0);
		}	
    }
	
    //IPv4 Addressing
    if(ipv4==1)
    {
    //Create Socket 
        if((clientfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
        {
            perror("[Error]Socket -- ");
            exit(EXIT_FAILURE);
        }
        //Initialise Socket address 
        struct sockaddr_in serveraddr;
        memset(&serveraddr,0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET; 
        serveraddr.sin_port = htons(portnumber);
        serveraddr.sin_addr.s_addr = inet_addr(ipaddr);   //Binding IPv4 address- Collected from user
        printf("\nClient Username :%s",username);
        printf("\nPort number :%u", portnumber);
        printf("\nBounded to IP :%s\n", ipaddr);
		
		//connecting to server
        if(connect(clientfd, (struct sockaddr *) &serveraddr,sizeof(serveraddr))==-1)
        {
            close(clientfd);
            perror("client : connect");
            exit(1);
        }
        else
        {
          
		//packing and sending username of client  
        buffs=join_server(username,MSG_JOIN,ATTR_USERNAME);
        if(send(clientfd, buffs, MAXLINE, MSG_DONTWAIT)==-1)    
        {
            perror("[Error] No data sent \n closing socket");
            close(clientfd);
        }
        free(buffs);
        }
    }
    //IPv6 Addressing
    else
    {
        char ipstr[INET6_ADDRSTRLEN];
        //Assigning server parameters
        for(p = servinfo; p != NULL; p = p->ai_next)
        {
            void *addr;
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
            strcpy(ipaddr,ipstr);
            //Creating socket  
            if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                perror("client: socket");
                continue;
            }

            printf("\nClient Username :%s",username);
            printf("\nPort number :%u", portnumber);
            printf("\nBounded to IP :%s\n", ipaddr);

            //Connecting
            if (connect(clientfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(clientfd);
                perror("client: connect");
                continue;
            }
            else
            {
                // printf("User <%s> connected to Server\n",username);
                buffs=join_server(username,MSG_JOIN,ATTR_USERNAME);
                if(write(clientfd, buffs, MAXLINE)<0)    //Sending user data
                {
                    perror("[Error] No data sent \n closing socket");
                    close(clientfd);
                }
                free(buffs);
            }
        }
    }
    //setting fd parameters
    FD_ZERO(&masterfds);
    FD_ZERO(&readfds);
    FD_SET(fileno(stdin),&masterfds);
    FD_SET(clientfd,&masterfds);

    //Sending and Receiving message 
    while(1)
    {
        readfds=masterfds;
		//select() using readfds and timeout
        select_value = select(clientfd+1,&readfds,NULL,NULL,&tc);
        if(select_value==-1)
        {
            perror("[Error] Select--");
            close(clientfd);
            exit(0);
        }
        if(select_value==0)
        {
            buffs = join_server(" ",MSG_IDLE,0);
            send_bytes=send(clientfd, buffs, MAXLINE, MSG_DONTWAIT);   //Sending user data
            if(send_bytes==-1) 
            {
                perror("[Error] No data sent \n closing socket");
                close(clientfd);
            }
            free(buffs);
			//resetting timer
            tc.tv_sec = timeout_value;
         }
        //Reading from user - STDIN
        if(FD_ISSET(fileno(stdin),&readfds))
        {
            if(fgets(buff, MAXLINE, stdin)!=NULL)   //Get data from user
            {
                tc.tv_sec = timeout_value;
				buffs = join_server(buff, MSG_SEND,ATTR_MESSAGE);     
                send_bytes=send(clientfd, buffs, MAXLINE, MSG_DONTWAIT);   //Sending user data
                if(send_bytes==-1) 
                {
                    perror("[Error] No data sent \n closing socket");
                    close(clientfd);
                }
                free(buffs);
            }
            else
            {
                perror("[Error] EOF detected : Closing socket... Exiting");
                close(clientfd);
                exit(EXIT_FAILURE);
            }
        }
        //Reading on socket from server
        if(FD_ISSET(clientfd,&readfds))
        {
            length = read(clientfd, buffr, MAXLINE);
            if(length==0)
            {
                perror("[Error] receiving--");
                close(clientfd);
            }
            else
            {
                struct sbcp_message msg_test;
                struct sbcp_attribute msg_test_attr;
                msg_test.msg_attribute = &msg_test_attr;
                msg_test_attr.payload = malloc(512);
                msg_test_attr.type = 0;
                msg_test_attr.length = 0;
                msg_test.vrsn = 0;
                msg_test.type = 0;
                msg_test.length = 0;
                msg_test.msg_attribute = &msg_test_attr;

                char* p_0 = buffr;

                while (p_0 != NULL) {
                    p_0 = sbcp_pkt_tcp_segmentation(p_0, &msg_test);
                    
					//Comparing MSG Type and ATTR type for identifying type of message
                    // ACK
                    if(msg_test.type == MSG_ACK){
                        if(msg_test_attr.type == ATTR_COUNTC)
                            printf("\nNumber of online users: %s\n", msg_test_attr.payload);
                        if(msg_test_attr.type == ATTR_USERNAME)
                            printf("User list: %s\n\n", msg_test_attr.payload);
                    }
                    //NACK
                    if(msg_test.type == MSG_NAK && msg_test.msg_attribute->type == ATTR_REASON){
                        printf("Connection Refused : %s", msg_test.msg_attribute->payload);
                        close(clientfd);
                        exit(0);
                    }
                    // ONLINE
                    if(msg_test.type == MSG_ONLINE && msg_test_attr.type == ATTR_USERNAME){
                        printf("\nUser [%s] is online!\n", msg_test_attr.payload);
                    }
                    // OFFLINE
                    if(msg_test.type == MSG_OFFLINE && msg_test_attr.type == ATTR_USERNAME){
                        printf("\nUser [%s] is offline!\n", msg_test_attr.payload);
                    }
                    // IDLE
                    if(msg_test.type == MSG_IDLE && msg_test_attr.type == ATTR_USERNAME){
                        printf("\nUser [%s] is idle!\n", msg_test_attr.payload);
                    }
                    //FWD
                    if(msg_test.type == MSG_FWD && msg_test.msg_attribute->type == ATTR_USERNAME){
                        printf("\n| %s |", msg_test.msg_attribute->payload);
                    }
                    if(msg_test.type == MSG_FWD && msg_test.msg_attribute->type == ATTR_MESSAGE){
                        printf(" %s", msg_test.msg_attribute->payload);
                    }
                }
				//clearing buffer memory
                free(msg_test_attr.payload);
                memset(buffr, 0, sizeof(buffr));
            }
        }
    }
}
