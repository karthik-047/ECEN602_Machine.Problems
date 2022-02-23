#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include "sbcp_protocol.h"
#include "Server.h"
#include "Server_sbcp.h"

int check_user(struct sbcp_message* recv_msg, int* curr_client, int max_client, struct user* db, int connect_fd, int max_fd){
    if(recv_msg->type != MSG_JOIN || recv_msg->msg_attribute->type != ATTR_USERNAME){
        printf("[INVALID CONNECTION] Wrong message type.\n");
        return -1;
    }
    else if((*curr_client) == max_client ){
        printf("[INVALID CONNECTION] Current the number of clients reach the limit. Connection Denied.\n");
        return -2;
    }
    else{
        char username[MAXLINE];
        strcpy(username, recv_msg->msg_attribute->payload);
        int chk_db = check_name_db(username, db, max_fd);
        if(chk_db < 0) {
            return chk_db;
        }
        else{
            printf("\n**************************************************\n");
            add_user_db(username, db, curr_client, connect_fd);
            return 1;
        }
    }
}

// Create an array to hold usernames of connected clients
struct user* create_user_db(int max_client){
    struct user* db = (struct user*)malloc(max_client*(sizeof(struct user)));
    for(int i = 0; i < max_client; i++){
        //db[i].name = (char*)malloc(sizeof(char) * 16);
        strcpy(db[i].name, "\0");
        db[i].fd = 0;
    }
    return db;
}

// Check if one name is existed in current database
int check_name_db(char* name, struct user* db, int max_fd){
    if(strlen(name) > 16){
        printf("[INVALID CONNECTION] The user name is oversize.\n");
        return -3;
    }
    if (max_fd > 0){
        for(int i = 0; i <= max_fd; i++){
            if(strcmp(db[i].name, name) == 0){
                printf("[INVALID CONNECTION] The user name is already existed.\n");
                return -4;
            }
        }
    }
    return 1;
}

// Add the new username to the database
// Pass the reference of the variable of current number of clients to the function
void add_user_db(char* name, struct user* db, int* curr_client, int connect_fd){
    //strcpy(db[*curr_client].name, name);
    //db[*curr_client].fd = connect_fd;
    //printf("[NEW USER ADDED] Name: %s; FD: %d;\n", db[*curr_client].name, db[*curr_client].fd);
    strcpy(db[connect_fd].name, name);
    db[connect_fd].fd = connect_fd;
    printf("[NEW USER ADDED] Name: %s; FD: %d;\n", db[connect_fd].name, db[connect_fd].fd);
    (*curr_client)++;
    printf("[NEW USER ADDED] Current the number of users: %d\n", *curr_client);
}

// Remove the username from the database
void remove_user_db(struct user* db, int* curr_client, int connect_fd, int max_fd){
    //while(db[i].fd != connect_fd){
    //    i++;
    //}
    printf("\n**************************************************\n");
    printf("[REMOVE USER] Name: %s; FD: %d;\n", db[connect_fd].name, db[connect_fd].fd);
    // while(i < max_fd){
    //    db[i] = db[i+1];
    //    i++;
    //}

    db[connect_fd].fd = 0;
    strcpy(db[connect_fd].name, "\0");

    (*curr_client)--;
    printf("[REMOVE USER] Current the number of users: %d\n", *curr_client);
    printf("**************************************************\n\n");
}

int sbcp_SEND_server(char* buff, char* message){
    struct sbcp_message send_msg;
    struct sbcp_attribute send_attr;
    send_msg.msg_attribute = &send_attr;
    sbcp_unpack(buff, &send_msg);

    if(send_msg.type == MSG_IDLE){
        return -1;
    }

    if(send_msg.type != MSG_SEND || send_attr.type != ATTR_MESSAGE){
        printf("[INVALID MESSAGE] Wrong message/attribute type.\n");
        return -2;
    }
    else{
        strcpy(message, send_attr.payload);
        return 1;
    }
}

// Broadcast message with type of FWD
void sbcp_FWD(char* name, char* message, int listen_fd, int connect_fd, int max_fd, fd_set* all_set){
    // Packet 1: Send username
    char* username = malloc(strlen(name) + 1);
    strcpy(username, name);
    struct sbcp_message fwd_name;
    struct sbcp_attribute fwd_name_attr;
    fwd_name.msg_attribute = &fwd_name_attr;
    fwd_name_attr.payload = username;
    fwd_name.msg_attribute->type = ATTR_USERNAME;
    fwd_name.msg_attribute->length = strlen(name) + 4;
    fwd_name.vrsn = SBCP_VRSN;
    fwd_name.type = MSG_FWD;
    fwd_name.length = fwd_name.msg_attribute->length + 4;
    // Packet 2: Send forward message
    struct sbcp_message fwd_msg;
    struct sbcp_attribute fwd_msg_attr;
    fwd_msg.msg_attribute = &fwd_msg_attr;
    fwd_msg_attr.payload = message;
    fwd_msg.msg_attribute->type = ATTR_MESSAGE;
    fwd_msg.msg_attribute->length = strlen(message) + 4;
    fwd_msg.vrsn = SBCP_VRSN;
    fwd_msg.type = MSG_FWD;
    fwd_msg.length = fwd_msg.msg_attribute->length + 4;

    unsigned char* send_pkt_name = sbcp_pack(&fwd_name);
    unsigned char* send_pkt_msg = sbcp_pack(&fwd_msg);

    for(int i = 0; i <= max_fd; i++){
        // 1. Check if current fd is valid
        if(FD_ISSET(i, all_set)){
            // 2. Check if current fd is a client fd
            if(i != listen_fd && i != connect_fd){
                if(write(i, send_pkt_name, fwd_name.length) < 0){
                    perror("[Error] Send(FWD NAME) -- ");
                    printf("[Error] FD: %d\n", i);
                    //exit(FAILURE);
                }
                if(write(i, send_pkt_msg, fwd_msg.length) < 0){
                    perror("[Error] Send(FWD MSG) -- ");
                    printf("[Error] FD: %d\n", i);
                    //exit(FAILURE);
                }
            }  // Check 2
        } // Check 1
    }
    free(username);
}

int sbcp_JOIN_server(int listen_fd, int connect_fd, fd_set* read_set, unsigned char* buff, int* curr_client, int max_client, int max_fd, struct user* db){
    // Initialize buffer for JOIN
    memset(buff, 0, MAXLINE);
    int recv_bytes = read(connect_fd, buff, MAXLINE);

    if(recv_bytes > 0){
        // Unpack the data stream into a message struct
        struct sbcp_message join_msg;
        struct sbcp_attribute join_attr;
        join_msg.msg_attribute = &join_attr;
        sbcp_unpack(buff, &join_msg);

        return check_user(&join_msg, curr_client, max_client, db, connect_fd, max_fd);
    }
    else{
        printf("[INVALID CONNECTION] Received data stream is invalid.\n");
        return 0;
    }
}


void sbcp_ACK(int listen_fd, int connect_fd, int max_fd, int curr_client, struct user* db, fd_set* all_set){
    // Packets with the number of current clients
    char* count = malloc(sizeof(char) * 5);
    sprintf(count, "%d", curr_client);

    struct sbcp_message ack_msg_cc;
    struct sbcp_attribute ack_msg_cc_attr;
    ack_msg_cc.msg_attribute = &ack_msg_cc_attr;
    ack_msg_cc_attr.payload = count;
    ack_msg_cc_attr.type = ATTR_COUNTC;
    ack_msg_cc_attr.length = strlen(ack_msg_cc_attr.payload) + 4;
    ack_msg_cc.vrsn = SBCP_VRSN;
    ack_msg_cc.type = MSG_ACK;
    ack_msg_cc.length = ack_msg_cc_attr.length + 4;

    printf("[DEBUG]Count : %s\n", ack_msg_cc_attr.payload);

    // Packets with the names of existed users
    char* namelist = malloc(sizeof(char) * MAXLINE);
    int cat_j = 0;
    for(int i = 0; i <= max_fd; i++){
        // 1. Check if current fd is valid
        if(FD_ISSET(i, all_set)){
            // 2. Check if current fd is a client fd
            if(i != listen_fd && i != connect_fd){
                cat_j += sprintf(namelist + cat_j, "%s ", db[i].name);
            }  // Check 2
        } // Check 1
    }

    struct sbcp_message ack_msg_name;
    struct sbcp_attribute ack_msg_name_attr;
    ack_msg_name.msg_attribute = &ack_msg_name_attr;
    ack_msg_name_attr.payload = namelist;
    ack_msg_name_attr.type = ATTR_USERNAME;
    ack_msg_name_attr.length = strlen(ack_msg_name_attr.payload) + 4;
    ack_msg_name.vrsn = SBCP_VRSN;
    ack_msg_name.type = MSG_ACK;
    ack_msg_name.length = ack_msg_name_attr.length + 4;

    printf("[DEBUG]Namelist : %s\n", ack_msg_name_attr.payload);

    // Pack and Send
    unsigned char* send_cc = sbcp_pack(&ack_msg_cc);
    unsigned char* send_name = sbcp_pack(&ack_msg_name);

    if(write(connect_fd, send_cc, ack_msg_cc.length) < 0){
        perror("[Error] Send(ACK CC) -- ");
        printf("[Error] FD: %d\n", connect_fd);
    }
    if(write(connect_fd, send_name, ack_msg_name.length) < 0){
        perror("[Error] Send(ACK NAME) -- ");
        printf("[Error] FD: %d\n", connect_fd);
    }

    free(count);
    free(namelist);

}


void sbcp_NAK(int connect_fd, int check_flag){
    // printf("[DEBUG] Check Flag = %d\n", check_flag);
    char reason[128] = {0};
    if(check_flag == -1)
        strcpy(reason, "Invalid packet type for a join request.");
    if(check_flag == -2)
        strcpy(reason, "The number of existed clients reaches the limit.");
    if(check_flag == -3)
        strcpy(reason, "The username is oversize.");
    if(check_flag == -4)
        strcpy(reason, "The username is already existed.");

    struct sbcp_message nak_msg;
    struct sbcp_attribute nak_msg_attr;
    nak_msg.msg_attribute = &nak_msg_attr;
    nak_msg_attr.payload = reason;
    nak_msg_attr.type = ATTR_REASON;
    nak_msg_attr.length = strlen(nak_msg_attr.payload) + 4;
    nak_msg.vrsn = SBCP_VRSN;
    nak_msg.type = MSG_NAK;
    nak_msg.length = nak_msg_attr.length + 4;

    printf("[DEBUG]Reason: %s\n", nak_msg_attr.payload);

    unsigned char* send_reason = sbcp_pack(&nak_msg);
    if(write(connect_fd, send_reason, nak_msg.length) < 0){
        perror("[Error] Send(NAK) -- ");
        printf("[Error] FD: %d\n", connect_fd);
    }
}

void sbcp_ONLINE(int listen_fd, int connect_fd, int max_fd, struct user* db, fd_set* all_set){

    struct sbcp_message online_msg;
    struct sbcp_attribute online_msg_attr;
    online_msg.msg_attribute = &online_msg_attr;
    online_msg_attr.payload = db[connect_fd].name;
    online_msg_attr.type = ATTR_USERNAME;
    online_msg_attr.length = strlen(online_msg_attr.payload) + 4;
    online_msg.vrsn = SBCP_VRSN;
    online_msg.type = MSG_ONLINE;
    online_msg.length = online_msg_attr.length + 4;

    unsigned char* send_online = sbcp_pack(&online_msg);

    for(int i = 0; i <= max_fd; i++){
        // 1. Check if current fd is valid
        if(FD_ISSET(i, all_set)){
            // 2. Check if current fd is a client fd
            if(i != listen_fd && i != connect_fd){
                if(write(i, send_online, online_msg.length) < 0){
                    perror("[Error] Send(ONLINE) -- ");
                    printf("[Error] FD: %d\n", i);
                    //exit(FAILURE);
                }
            }  // Check 2
        } // Check 1
    }
}

void sbcp_OFFLINE(int listen_fd, int connect_fd, int max_fd, struct user* db, fd_set* all_set){

    struct sbcp_message offline_msg;
    struct sbcp_attribute offline_msg_attr;
    offline_msg.msg_attribute = &offline_msg_attr;
    offline_msg_attr.payload = db[connect_fd].name;
    offline_msg_attr.type = ATTR_USERNAME;
    offline_msg_attr.length = strlen(offline_msg_attr.payload) + 4;
    offline_msg.vrsn = SBCP_VRSN;
    offline_msg.type = MSG_OFFLINE;
    offline_msg.length = offline_msg_attr.length + 4;

    unsigned char* send_offline = sbcp_pack(&offline_msg);

    for(int i = 0; i <= max_fd; i++){
        // 1. Check if current fd is valid
        if(FD_ISSET(i, all_set)){
            // 2. Check if current fd is a client fd
            if(i != listen_fd && i != connect_fd){
                if(write(i, send_offline, offline_msg.length) < 0){
                    perror("[Error] Send(OFFLINE) -- ");
                    printf("[Error] FD: %d\n", i);
                    //exit(FAILURE);
                }
            }  // Check 2
        } // Check 1
    }
}


void sbcp_IDLE(int listen_fd, int connect_fd, int max_fd, struct user* db, fd_set* all_set){

    struct sbcp_message idle_msg;
    struct sbcp_attribute idle_msg_attr;
    idle_msg.msg_attribute = &idle_msg_attr;
    idle_msg_attr.payload = db[connect_fd].name;
    idle_msg_attr.type = ATTR_USERNAME;
    idle_msg_attr.length = strlen(idle_msg_attr.payload) + 4;
    idle_msg.vrsn = SBCP_VRSN;
    idle_msg.type = MSG_IDLE;
    idle_msg.length = idle_msg_attr.length + 4;

    unsigned char* send_idle = sbcp_pack(&idle_msg);

    for(int i = 0; i <= max_fd; i++){
        // 1. Check if current fd is valid
        if(FD_ISSET(i, all_set)){
            // 2. Check if current fd is a client fd
            if(i != listen_fd && i != connect_fd){
                if(write(i, send_idle, idle_msg.length) < 0){
                    perror("[Error] Send(IDLE) -- ");
                    printf("[Error] FD: %d\n", i);
                    //exit(FAILURE);
                }
            }  // Check 2
        } // Check 1
    }
}
