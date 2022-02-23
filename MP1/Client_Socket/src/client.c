#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"

#define MAXLINE 20

int main(int argc, char *argv[])
{
   if (argc != 3)
	{
    fprintf(stderr, "usage: %s <serverip> <port> \n", argv[0]);
    exit(1);
    }
	
   uint16_t portnumber;   
   int clientfd;                       //File Descriptor for socket
   struct sockaddr_in serveraddr;
   char buff[MAXLINE];
   char detect[MAXLINE] = "exitclient\n";  //Codeword for Testcase #4
   int length;
   
   portnumber = atoi(argv[2]);         //Portnumber - collected from user
  
   printf(" \n Port number : %u \n", portnumber);
   printf("\n Bounded to IP : %s \n", argv[1]);
   
   //Create Socket	
   if((clientfd = socket(PF_INET, SOCK_STREAM, 0))<0)
   {
	   perror("[Error]Socket -- ");
       exit(EXIT_FAILURE);
    }
   //Initialise Socket address	
   memset(&serveraddr,0, sizeof(serveraddr));
   serveraddr.sin_family = AF_INET;
   serveraddr.sin_port = htons(portnumber);
   serveraddr.sin_addr.s_addr = inet_addr(argv[1]);   //Binding IPv4 address- Collected from user
   
   //Connect with Server
   if(connect(clientfd, (struct sockaddr *) &serveraddr,sizeof(serveraddr))==-1)
   {
	   close(clientfd);
	   perror("client : connect");
	   
   }
   else
   {
	   printf("Connected to Server\n");
   }
   while(1)
   {
      printf("Type your message :" );
      if(fgets(buff, MAXLINE, stdin)!=NULL)   //Get data from user
      {                           
         if(send(clientfd, buff, MAXLINE, MSG_DONTWAIT)==-1)    //Sending user data
	  {
		  perror("[Error] No data sent \n closing socket");
		  close(clientfd);
	  }
	  if(strcmp(detect, buff)==0)                              //Checking terminate command
	  {
		  perror("[Error] Exit Command detected..terminating");
		  close(clientfd);
		  exit(1);
	  }
      length = read(clientfd, buff, MAXLINE);
      buff[length] ='\0';
      printf("Echo from Server:");
      read_newline(buff);
      
     }
     else
     {
      perror("[Error] EOF detected : Closing socket... Exiting");
      close(clientfd);
      exit(1);
     } 
   } 
}
