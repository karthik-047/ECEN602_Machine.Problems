#ifndef SERVER_TFTP_H
#define SERVER_TFTP_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include "Server.h"

#define LIB             "./File_Vault/"
#define CLIENT_SPACE    "./Client_Space/"

#define TIMEOUT     5
#define MAX_RESEND  5

#define BLOCKSIZE       512
#define FILENAME_SIZE   64
// Opcode
#define RRQ_OP      1
#define WRQ_OP      2
#define DATA_OP     3
#define ACK_OP      4
#define ERROR_OP    5

#define ERR_FREE            -1
#define ERR_NOT_DEFINED     0
#define ERR_FILE_NOT_FOUND  1
#define ERR_ACCESS_VIOLATE  2
#define ERR_DISK_FULL       3
#define ERR_ILLEGAL_OP      4
#define ERR_UNKNOWN_TXID    5
#define ERR_FILE_EXIST      6
#define ERR_NO_SUCH_USER    7

typedef struct tftp_req_msg{
    uint16_t opcode;
    char     filename[FILENAME_SIZE];
    char     mode[16];
}tftp_req_msg_t;

typedef struct tftp_data_msg{
    uint16_t opcode;
    uint16_t block_num;
    unsigned char     data[BLOCKSIZE];
}tftp_data_msg_t;

typedef struct tftp_ack_msg{
    uint16_t opcode;
    uint16_t block_num;
}tftp_ack_msg_t;

typedef struct tftp_error_msg{
    uint16_t opcode;
    uint16_t errcode;
    char     err_msg[64];
}tftp_error_msg_t;

uint16_t byteswap_16(int);
void tftp_gen_data_msg(uint16_t, char*, tftp_data_msg_t*, int);
int tftp_pack_data_msg(unsigned char*, tftp_data_msg_t, int);
void tftp_pack_ack_msg(unsigned char*, uint16_t);
void tftp_gen_error_msg(int, char*, tftp_error_msg_t*);
int tftp_pack_error_msg(unsigned char*, tftp_error_msg_t);
int check_req(unsigned char*, tftp_req_msg_t*);
int check_data_recv(unsigned char*, char*, int, int);
int check_ack_recv(unsigned char*, uint16_t);
int readable_timeo(int);
int tftp_read_send(char*, unsigned char*, int, uint16_t, int, struct sockaddr_in, socklen_t);
int tftp_write_recv(char*, unsigned char*, int, uint16_t, int, struct sockaddr_in, socklen_t);
void tftp_read_octet(FILE** , unsigned char*, int, struct sockaddr_in, socklen_t);
void tftp_write_octet(FILE**, unsigned char*, int, struct sockaddr_in, socklen_t);
int tftp_read(char*, char*, char*, int, struct sockaddr_in, socklen_t);
int tftp_write(char*, char*, char*, int, struct sockaddr_in, socklen_t);
int tftp_transfer(tftp_req_msg_t, unsigned char*, struct sockaddr_in, socklen_t, struct sockaddr_in);

#endif
