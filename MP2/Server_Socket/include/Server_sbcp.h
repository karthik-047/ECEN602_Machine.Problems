#ifndef SERVER_SBCP_H
#define SERVER_SBCP_H

#include <stdint.h>

struct user{
    int fd;
    char name[16];
};


struct user* create_user_db(int);
int check_name_db(char*, struct user*, int);
void add_user_db(char*, struct user*, int*, int);
void remove_user_db(struct user*, int*, int, int);

int check_user(struct sbcp_message*, int*, int, struct user*, int, int);

int sbcp_SEND_server(char*, char*);
void sbcp_FWD(char*, char*, int, int, int, fd_set*);
int sbcp_JOIN_server(int, int, fd_set*, unsigned char*, int*, int, int, struct user*);
void sbcp_ACK(int, int, int, int, struct user*, fd_set*);
void sbcp_NAK(int, int);
void sbcp_ONLINE(int, int, int, struct user*, fd_set*);
void sbcp_OFFLINE(int, int, int, struct user*, fd_set*);
void sbcp_IDLE(int, int, int, struct user*, fd_set*);

#endif
