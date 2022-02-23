#include "Server_http.h"

char *WEEKDAY[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *MONTH[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int http_req_parse(char* buf, http_req_t* req){
    // printf("[DEBUG] Requset message from client: \n%s\n", buf);
    int ret = sscanf(buf, "%s %s %s Host: %s\r\n\r\n", req->method, req->file, req->protocol, req->host);
    // printf("[DEBUG] Request method: %s\n", req->method);
    // printf("[DEBUG] Request file: %s\n", req->file);
    // printf("[DEBUG] Request protocol: %s\n", req->protocol);
    // printf("[DEBUG] Request host: %s\n", req->host);
    if(strcmp(req->method, "GET") != 0){
        printf("[Error] Unsupported command : %s\n", req->method);
        return -1;
    }
    if(strcmp(req->protocol, "HTTP/1.0") != 0){
        printf("[Error] Unsupported protocol : %s\n", req->protocol);
        return -1;
    }

    return ret;
}

int connect_server(http_req_t req){
    int serv_fd;
    struct addrinfo hints, *result;

    if((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("[Error] Socket to remote -- ");
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(req.host, "80", &hints, &result) != 0){
        printf("[Error] Failed to run getaddrinfo.\n");
        return -1;
    }

    if(connect(serv_fd, result->ai_addr, result->ai_addrlen) < 0){
        perror("[Error] Connect to remote -- ");
        close(serv_fd);
        return -1;
    }

    return serv_fd;
}

int send_http_get(int serv_fd, char* buf, http_req_t req){
    memset(buf, 0, MAXLINE);
    sprintf(buf, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", req.file, req.host);
    // printf("[DEBUG] Send request:\n%s \n", buf);

    int send_len;
    if((send_len = send(serv_fd, buf, strlen(buf), 0)) < 0){
        perror("[Error] Send to remote -- ");
        return -1;
    }
    return send_len;
}

int send_http_get_hit(int serv_fd, char* buf, http_cache_t* cache, int req_cache_index){
    memset(buf, 0, MAXLINE);
    if(!strcmp(cache[req_cache_index].time_expires, "0")){
        sprintf(buf, "GET %s HTTP/1.0\r\nHost: %s\r\nIf-Modified-Since: %s\r\n\r\n", cache[req_cache_index].file, cache[req_cache_index].host, cache[req_cache_index].time_last_accessed);
    }
    else{
        sprintf(buf, "GET %s HTTP/1.0\r\nHost: %s\r\nIf-Modified-Since: %s\r\n\r\n", cache[req_cache_index].file, cache[req_cache_index].host, cache[req_cache_index].time_expires);
    }
    // printf("[DEBUG] Send request:\n%s \n", buf);

    int send_len;
    if((send_len = send(serv_fd, buf, strlen(buf), 0)) < 0){
        perror("[Error] Send to remote -- ");
        return -1;
    }
    return send_len;
}

int recv_http_get(int serv_fd, char* buf, char* header, char* name){
    FILE *fp;
    int recv_len;

    if((fp = fopen(name, "w+")) == NULL){
        printf("[Error] Cannot create a new file: [%s].\n", name);
        return -1;
    }

    // Get the response header
    char* header_end;
    while(1){
        memset(buf, 0, MAXLINE);
        recv_len = recv(serv_fd, buf, MAXLINE, 0);
        if((header_end = strstr(buf, "\r\n\r\n")) != NULL){
            // printf("[DEBUG] CRLF Matched.\n");
            strncat(header, buf, header_end - buf + 1);
            if(header_end < buf + MAXLINE - 4){
                // printf("[DEBUG] CRLF - Short\n");
                fwrite(header_end + 4, 1, MAXLINE - (header_end + 4 - buf), fp);
                // printf("[DEBUG] Bytes read: %d\n", recv_len);
                // printf("[DEBUG] %s\n", buf);
            }
            break;
        }
        if((header_end = strstr(buf, "\n\n")) != NULL){
            // printf("[DEBUG] LF Matched.\n");
            strncat(header, buf, header_end - buf + 1);
            if(header_end < buf + MAXLINE - 2){
                // printf("[DEBUG] LF - Short\n");
                fwrite(header_end + 2, 1, MAXLINE - (header_end + 2 - buf), fp);
                // printf("[DEBUG] Bytes read: %d\n", recv_len);
                // printf("[DEBUG] %s\n", buf);
            }
            break;
        }
        strncat(header, buf, MAXLINE);
    }

    memset(buf, 0, MAXLINE);
    while((recv_len = recv(serv_fd, buf, MAXLINE, 0)) > 0){
        // printf("[DEBUG] Bytes read: %d\n", recv_len);
        // printf("[DEBUG] %s\n", buf);
        fwrite(buf, 1, recv_len, fp);
        memset(buf, 0, MAXLINE);
    }

    fclose(fp);

    return 1;

}

int recv_http_get_hit(int serv_fd, char* buf, char* header, char* name){
    FILE *fp;
    int recv_len;
    char temp_buf[MAXLINE];
    int data_attach_len = 0;

    // Get the response header
    char* header_end;
    while(1){
        memset(buf, 0, MAXLINE);
        recv_len = recv(serv_fd, buf, MAXLINE, 0);
        if((header_end = strstr(buf, "\r\n\r\n")) != NULL){
            // printf("[DEBUG] CRLF Matched.\n");
            strncat(header, buf, header_end - buf + 1);
            if(header_end < buf + MAXLINE - 4){
                // printf("[DEBUG] CRLF - Short\n");
                data_attach_len = MAXLINE - (header_end + 4 - buf);
                memcpy(temp_buf, header_end + 4, data_attach_len);
                // printf("[DEBUG] Bytes read: %d\n", recv_len);
                // printf("[DEBUG] %s\n", buf);
            }
            break;
        }
        if((header_end = strstr(buf, "\n\n")) != NULL){
            // printf("[DEBUG] LF Matched.\n");
            strncat(header, buf, header_end - buf + 1);
            if(header_end < buf + MAXLINE - 2){
                // printf("[DEBUG] LF - Short\n");
                data_attach_len = MAXLINE - (header_end + 2 - buf);
                memcpy(temp_buf, header_end + 2, data_attach_len);
                // printf("[DEBUG] Bytes read: %d\n", recv_len);
                // printf("[DEBUG] %s\n", buf);
            }
            break;
        }
        strncat(header, buf, MAXLINE);
    }

    if(strstr(header, "304 Not Modified") != NULL){
        printf("[INFO] File is not modified.\n");
        return 1;
    }

    if(strstr(header, "200 OK") != NULL){
        printf("[INFO] File updated.\n");
        if((fp = fopen(name, "w+")) == NULL){
            printf("[Error] Cannot create a new file.\n");
            return -1;
        }
        if(data_attach_len > 0){
            fwrite(temp_buf, 1, data_attach_len, fp);
        }

        memset(buf, 0, MAXLINE);
        while((recv_len = recv(serv_fd, buf, MAXLINE, 0)) > 0){
            // printf("[DEBUG] Bytes read: %d\n", recv_len);
            // printf("[DEBUG] %s\n", buf);
            fwrite(buf, 1, recv_len, fp);
            memset(buf, 0, MAXLINE);
        }

        fclose(fp);
    }
    return 0;

}

int month_convert(char* month){
    for(int i = 0; i < 12; i++){
        if(!strcmp(month, MONTH[i])){
            return i;
        }
    }
    printf("[Error] Invalid month info.\n");
    return -1;
}

int weekday_convert(char* wday){
    for(int i = 0; i < 7; i++){
        if(!strcmp(wday, WEEKDAY[i])){
            return i;
        }
    }
    printf("[Error] Invalid weekday info.\n");
    return -1;
}

void time_transform_tm2str(struct tm* timetm, char* timestr){
    memset(timestr, 0, TIME_L);
    strftime(timestr, TIME_L, "%a, %d %b %Y %H:%M:%S GMT", timetm);
}

void time_transform_str2tm(struct tm* timetm, char* timestr){
    // printf("[DEBUG] Input time: %s\n", timestr);
    strptime(timestr, "%a, %d %b %Y %H:%M:%S GMT", timetm);
}


uint64_t timediff(char* time_ref, char* timestr){
    time_t basetime;
    struct tm* baseinfo = (struct tm*)malloc(sizeof(struct tm));
    struct tm* Intimeinfo = (struct tm*)malloc(sizeof(struct tm));
    if(time_ref == NULL){
        time(&basetime);
        baseinfo = gmtime(&basetime);
    }
    else{
        time_transform_str2tm(baseinfo, time_ref);
    }

    basetime = mktime(baseinfo);

    char base_str[TIME_L];
    time_transform_tm2str(baseinfo, base_str);
    // printf("[DEBUG] Base time: %s\n", base_str);

    char itime_str[TIME_L];
    time_transform_str2tm(Intimeinfo, timestr);
    // printf("[DEBUG] Input time: %s\n", timestr);

    time_t Intime = mktime(Intimeinfo);
    // printf("[DEBUG] Ref epoch: %ld\n", basetime);
    // printf("[DEBUG] Input epoch: %ld\n", Intime);

    free(baseinfo);
    free(Intimeinfo);

    return (Intime - basetime);
}

// Allocate space for the cache and initializa it
http_cache_t* cache_init(){
    http_cache_t* cache = (http_cache_t*)malloc(MAX_ENTRIES * (sizeof(http_cache_t)));
    memset(cache, 0, MAX_ENTRIES * (sizeof(http_cache_t)));
    // Assign initial LRU state of each cache line
    for(int i = 0; i < MAX_ENTRIES; i++){
        cache[i].LRU_state = i;
    }
    return cache;
}


int cache_access(http_req_t req, http_cache_t* cache, char* currtime){

    for(int i = 0; i < MAX_ENTRIES; i++){
        if(!strcmp(req.host, cache[i].host) && !strcmp(req.file, cache[i].file)){
            // printf("[DEBUG] Entry located. Expires: %s\n", cache[i].time_expires);
            // The cache page has a specific expire time
            if(!strcmp(cache[i].time_expires, "0")){
                printf("[INFO] Cache node missing Expires.\n");
                // A page can be considered "Fresh" when
                // The page is accesses in the preceding 24 hours
                // The page is last modified for over 30 days
                if(    (timediff(cache[i].time_last_accessed, currtime) < 3600 * 24) 
                    && (timediff(cache[i].time_last_modified, currtime) > 3600 * 24 *30))
                {
                    // Cache hit
                    return i;
                }
                // Take it as a cache miss since the page is "Stale"
                // Replace the current node
                else{
                    for(int j = 0; j < MAX_ENTRIES; j++){
                        if(cache[j].LRU_state < cache[i].LRU_state){
                            cache[j].LRU_state++;
                        }
                    }
                    cache[i].LRU_state = 0;
                    remove(cache[i].filepath);
                }
            }
            else{
                if(timediff(cache[i].time_expires, currtime) > 0){
                    printf("[INFO] Cache node expires.\n");
                    for(int j = 0; j < MAX_ENTRIES; j++){
                        if(cache[j].LRU_state < cache[i].LRU_state){
                            cache[j].LRU_state++;
                        }
                    }
                    cache[i].LRU_state = 0;
                    remove(cache[i].filepath);
                }
            }
            break;
        }
    }
    // Cache miss
    return -1;
}

int cache_access_mod(http_req_t req, http_cache_t* cache){

    for(int i = 0; i < MAX_ENTRIES; i++){
        if(!strcmp(req.host, cache[i].host) && !strcmp(req.file, cache[i].file)){
            return i;
        }
    }
    // Cache miss
    return -1;
}

// Add a new node into the cache based on LRU policy
int cache_lru_new(http_cache_t newline, http_cache_t* cache){
    int access_node;
    for(int i = 0; i < MAX_ENTRIES; i++){
        if(cache[i].LRU_state != 0){
            cache[i].LRU_state -= 1;
        }
        else{
            cache[i] = newline;
            access_node = i;
        }
    }
    print_cache_list(cache);
    return access_node;
}

// Hit an existed cache line
void cache_lru_hit(http_cache_t* cache, int cache_index){
    for(int i = 0; i < MAX_ENTRIES; i++){
        if(cache[i].LRU_state > cache[cache_index].LRU_state){
            cache[i].LRU_state -= 1;
        }
    }
    cache[cache_index].LRU_state = MAX_ENTRIES - 1;
    print_cache_list(cache);
}

void cache_lru_hit_mod(http_cache_t* cache, int cache_index){
    for(int i = 0; i < MAX_ENTRIES; i++){
        if(cache[i].LRU_state > cache[cache_index].LRU_state){
            cache[i].LRU_state -= 1;
        }
    }
    cache[cache_index].LRU_state = MAX_ENTRIES - 1;
    print_cache_list(cache);
}

// Check if the new page is cachable
// If yes, update the cache
int cache_new_page(char* header, http_req_t req, char* path, http_cache_t* cache){
    char* expires_h = strstr(header, "Expires");
    char* last_mod_h = strstr(header, "Last-Modified");

    // Not cacheable: Missing both Expires and Last-Modified
    if((expires_h == NULL) && (last_mod_h == NULL)){
        printf("[INFO] Page not cached.\n");
        return -1;
    }
    // Format (Expires: XXXX....)
    char expires[TIME_L] = {0};
    if(expires_h != NULL){
        if(sscanf(expires_h, "Expires: %[a-zA-z0-9 :,]", expires) < 0){
            printf("[Error] Missing timestamp for Expires.\n");
        }
        // printf("[DEBUG] Expires: %s\n", expires);
    }
    else{
        strcpy(expires, "0");
    }
    // Format (Last-Modified: XXXX....)
    char last_mod[TIME_L] = {0};
    if(last_mod_h != NULL){
        if(sscanf(last_mod_h, "Last-Modified: %[a-zA-z0-9 :,]", last_mod) < 0){
            printf("[Error] Missing timestamp for Last-Modified\n");
        }
        // printf("[DEBUG] Last-Modified: %s\n", last_mod);
    }
    else{
        strcpy(last_mod, "0");
    }

    // Format (Date: XXXX....)
    char* date_h = strstr(header, "Date");
    char date[TIME_L] = {0};
    if(date_h != NULL){
        if(sscanf(date_h, "Date: %[a-zA-z0-9 :,]", date) < 0){
            printf("[Error] Missing timestamp for Date\n");
        }
        // printf("[DEBUG] Date: %s\n", date);
    }
    else{
        strcpy(date, "0");
    }

    http_cache_t newnode;
    newnode.LRU_state = MAX_ENTRIES - 1;
    strcpy(newnode.host, req.host);
    strcpy(newnode.file, req.file);
    strcpy(newnode.filepath, path);
    strcpy(newnode.time_last_accessed, date);
    strcpy(newnode.time_last_modified, last_mod);
    strcpy(newnode.time_expires, expires);

    return(cache_lru_new(newnode, cache));
}

void print_cache_list(http_cache_t* cache){
    printf("==================PROXY SERVER CACHE====================\n\n");
    for(int i = 0; i < MAX_ENTRIES; i++){
        printf("*********************  NODE %d **********************\n", i);
        printf("[HOST] %s\n", cache[i].host);
        printf("[FILE] %s\n", cache[i].file);
        printf("[FILEPATH] %s\n", cache[i].filepath);
        printf("[TIME (Last Accessed)] %s\n", cache[i].time_last_accessed);
        printf("[TIME (Last Modified)] %s\n", cache[i].time_last_modified);
        printf("[TIME (Expires)]       %s\n", cache[i].time_expires);
        printf("[LRU STATE] %d\n", cache[i].LRU_state);
        printf("\n");
    }
    printf("========================================================\n\n");
}

// Modified filenames to avoid illegal characters in file paths
void name_mod(char* ori){
    int i = 0;
    while(ori[i] != '\0'){
        if(ori[i] == '/'){
            ori[i] = '-';
        }
        i++;
    }
}

int http_respond_client(http_cache_t* cache, int index, char* recv_file, int client_fd, char* buf){
    // Create and send response header
    char response_header[HEADER_L];
    memset(response_header, 0, HEADER_L);

    strcat(response_header, "HTTP/1.0 ");
    strcat(response_header, "200 OK\n");
    strcat(response_header, "\n");

    if(send(client_fd, response_header, strlen(response_header), 0) < 0){
        perror("[Error] Send response header -- ");
        return -1;
    }

    // Send the file to the client
    FILE* fp;
    if(index < 0){
        fp = fopen(recv_file, "r");
    }
    else{
        fp = fopen(cache[index].filepath, "r");
    }

    if(fp == NULL){
        // printf("[Error] File: %s\n", cache_node.filepath);
        perror("[Error] Can't open file -- ");
        return -1;
    }

    int read_bytes, sent_bytes;

    while(!feof(fp)){
        memset(buf, 0, MAXLINE);
        read_bytes = fread(buf, 1, MAXLINE, fp);
        // printf("[DEBUG] Bytes read: %d\n", read_bytes);
        if((sent_bytes = send(client_fd, buf, read_bytes, 0)) < 0){
            perror("[Error] Send body -- ");
            return -1;
        }
        // printf("[DEBUG] Bytes sent: %d\n", sent_bytes);
        // printf("[DEBUG] Data sent: %s\n", buf);
    }

    fclose(fp);

    if(index < 0){
        if(remove(recv_file) < 0){
            perror("[Error] remove -- ");
            exit(EXIT_FAILURE);
        }
    }

    printf("[INFO] Finish responsing to the client.\n");
    printf("------------------------------------------\n\n");
    return 1;
}






