#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#define __USE_XOPEN
#define _GNU_SOURCE

#include <time.h>
#include "Server.h"

#define MAX_ENTRIES 10

#define METHOD_L    16
#define FILENAME_L  256
#define PROTOCOL_L  16
#define HOSTNAME_L  128
#define TIME_L      64

#define HEADER_L    16*1024

typedef struct http_cache{
    char host[HOSTNAME_L];
    char file[FILENAME_L];
    char filepath[FILENAME_L];
    char time_last_accessed[TIME_L];
    char time_last_modified[TIME_L];
    char time_expires[TIME_L];
    uint8_t LRU_state;
}http_cache_t;

typedef struct http_req{
    char method[METHOD_L];
    char file[FILENAME_L];
    char protocol[PROTOCOL_L];
    char host[HOSTNAME_L];
}http_req_t;

int http_req_parse(char*, http_req_t*);
int connect_server(http_req_t);
int send_http_get(int, char*, http_req_t);
int send_http_get_hit(int, char*, http_cache_t*, int);
int recv_http_get(int, char*, char*, char*);
int recv_http_get_hit(int, char*, char*, char*);
void time_transform_tm2str(struct tm*, char*);
void time_transform_str2tm(struct tm*, char*);
uint64_t timediff(char*, char*);
http_cache_t* cache_init();
int cache_access(http_req_t, http_cache_t*, char*);
int cache_access_mod(http_req_t, http_cache_t*);
int cache_lru_new(http_cache_t, http_cache_t*);
void cache_lru_hit(http_cache_t*, int);
int cache_new_page(char*, http_req_t, char*, http_cache_t*);
void name_mod(char*);
void print_cache_list(http_cache_t*);
int http_respond_client(http_cache_t*, int, char*, int, char*);

#endif
