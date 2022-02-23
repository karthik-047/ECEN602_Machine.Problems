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

#define MAXLINE   2048
#define HEADER_L  16*1024

//Function to send all the data in the buffer
int sendall(int sockfd, char * send_buf, int buffsize){

    int bytes_left = buffsize;
    int bytes_sent = 0;
    while(bytes_left>0){
        bytes_sent = send(sockfd,send_buf,bytes_left,MSG_DONTWAIT);
        if(bytes_sent<0){
            if(errno==EINTR)
                continue;
            else
                return -1;
        }
        else{
            bytes_left -= bytes_sent;
            send_buf += bytes_sent;
        }
    }
    return buffsize;
}

int main(int argc, char *argv[]){

    //Command Line Parameters
    char url[MAXLINE];
    uint16_t portnumber;
    char ipaddr[50];


    //Checking usage
    if(argc!=4){
        fprintf(stderr, "usage: %s <serverip> <port> <URL>\n", argv[0]);
        exit(-1);
    }
	
	//template for GET
    char *message = "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n\0";
	
    portnumber = atoi(argv[2]);
    strcpy(ipaddr,argv[1]);
    strcpy(url,argv[3]);
    printf("\nRequested URL : %s",url); 
    int url_len, send_bytes,recv_len;
    int i;
    url_len = strlen(url);
    char *file;
    char host[100];
    char send_req[MAXLINE];
    char buf[MAXLINE];
    char header[HEADER_L] = {0};
    char file_name[MAXLINE];

    //comparision for copying host name
    if(strncmp(url,"http://",7)==0){
        for(i=0;i<(strlen(url)-7);i++){
            url[i]=url[7+i];
        }
        url[strlen(url)-7]='\0';
    }
    if(strncmp(url,"https://",8)==0){
        for(i=0;i<(strlen(url)-8);i++){
            url[i]=url[8+i];
        }
        url[strlen(url)-8]='\0';
    }
	
	//Parsing url for host name and filename
    strtok_r(url,"/",&file);
    strcpy(host,url);
    host[strlen(host)]='\0';
    file[strlen(file)]='\0';
    printf("\nPort number   : %u", portnumber);
    printf("\nBounded to IP : %s",argv[1]);
    printf("\nHost          : %s",host);
    printf("\nFilename      : %s\n",file);
	
	//Packing into GET request
    sprintf(send_req,"GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n",file,host);
    printf("\n%s\n",send_req);

    //Initialise Socket parameters
    int clientfd; 
    if((clientfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))<0){
        perror("[Error]Socket -- ");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr,0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_port = htons(portnumber);
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);   //Binding IPv4 address- Collected from user

	//Connecting to Proxy Server
    if(connect(clientfd, (struct sockaddr *) &serveraddr,sizeof(serveraddr))==-1){
            close(clientfd);
            perror("client : connect");
            exit(1);
    }
	
	//Sending Data
    send_bytes = sendall(clientfd,send_req,sizeof(send_req));
    if(send_bytes<0){
        perror("[Error] No data sent \n closing socket");
        close(clientfd);
    }

	
    FILE *fp;
    char* header_end;
    strcpy(file_name,file);
	int flag=0,j=0;
	char comp[] = ".html";
	char oldchar = '/';
    char newchar = '_';
	
	//Adding .html for filename if absent
	for(i=(strlen(file_name)-5);i<strlen(file_name);i++){
		if(file_name[i]==comp[j]){
			j++;
			continue;
		}
		else{
			flag=1;
			break;
		}
	}
	if(flag){
		strcat(file_name,comp);
	}
	
	//Replacing / by _
    for(i=0;i<strlen(file_name);i++){
        if(file_name[i]==oldchar){
            file_name[i]=newchar;
        }
    }

    if((fp = fopen(file_name, "w+")) == NULL){
        printf("[Error] Cannot create a new file.\n");
        return -1;
    }

	//Receiving header
    while(1){
        recv_len = recv(clientfd, buf, MAXLINE, 0);
        if((header_end = strstr(buf, "\r\n\r\n")) != NULL){
            strncat(header, buf, header_end - buf + 1);
            break;
        }
        if((header_end = strstr(buf, "\n\n")) != NULL){
            strncat(header, buf, header_end - buf + 1);
            break;
        }
        strncat(header, buf, MAXLINE);
    }

    printf("[INFO] Response header received: \n%s\n", header);
    memset(buf, 0, MAXLINE);
	
	//Receving content of file and storing in a file
    while((recv_len = recv(clientfd, buf, MAXLINE, 0)) > 0){
        fwrite(buf, 1, recv_len, fp);
        memset(buf, 0, MAXLINE);
    }
	printf("[INFO] File [%s] Received\n",file_name);
    fclose(fp);
}


