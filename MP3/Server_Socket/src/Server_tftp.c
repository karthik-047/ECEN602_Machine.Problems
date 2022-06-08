#include "Server_tftp.h"

// Integers present in little endian after using memset
// Need to change it to big endian
uint16_t byteswap_16(int num){
    return ((uint16_t)num >> 8) | ((uint16_t)num << 8);
}

// Generate TFTP data message
void tftp_gen_data_msg(uint16_t block, char* data_seg, tftp_data_msg_t* data_msg, int read_bytes){
    data_msg->opcode = DATA_OP;
    data_msg->block_num = block;
    memcpy(data_msg->data, data_seg, read_bytes);
}

// Make TFTP data packet
int tftp_pack_data_msg(unsigned char* buf, tftp_data_msg_t fileseg, int read_bytes){
    fileseg.opcode = byteswap_16(fileseg.opcode);
    memcpy(buf, &fileseg.opcode, 2);

    // printf("[DEBUG] Block : %x\n", fileseg.block_num);
    fileseg.block_num = byteswap_16(fileseg.block_num);
    // printf("[DEBUG] Block(swap) : %x\n", fileseg.block_num);
    memcpy(buf + 2, &fileseg.block_num, 2);

    memcpy(buf + 4, fileseg.data, read_bytes);
    // printf("[DEBUG] (PACK) read bytes : %d\n", read_bytes);
    // printf("[DEBUG] (PACK) First C : %d ; Last C : %d ; (MSG)\n", fileseg.data[0], fileseg.data[read_bytes - 1]);
    // printf("[DEBUG] (PACK) First C : %d ; Last C : %d ;\n", buf[4], buf[read_bytes + 3]);
    return (4 + read_bytes);
}


// Make TFTP ack packet
void tftp_pack_ack_msg(unsigned char* buf, uint16_t global_block_num){
    uint16_t opcode = ACK_OP;
    opcode = byteswap_16(opcode);
    memcpy(buf, &opcode, 2);

    uint16_t block_num = byteswap_16(global_block_num);
    memcpy(buf + 2, &block_num, 2);
}

//Assigning err opcode and err number
void tftp_gen_error_msg(int err_type,  char* info, tftp_error_msg_t* err){
    err->opcode = ERROR_OP;
    err->errcode = err_type;
    strcpy(err->err_msg, info);
}

// Make TFTP error packet
int tftp_pack_error_msg(unsigned char* buf, tftp_error_msg_t err){
    err.opcode = byteswap_16(err.opcode);
    memcpy(buf, &err.opcode, 2);

    err.errcode = byteswap_16(err.errcode);
    memcpy(buf + 2, &err.errcode, 2);

    memcpy(buf + 4, err.err_msg, strlen(err.err_msg));
    // Add trailing zero of the error packet
    uint8_t zero = 0;
    memcpy(buf + 4 + strlen(err.err_msg), &zero, 1);

    return (4 + strlen(err.err_msg));
}

int check_req(unsigned char* buf, tftp_req_msg_t* req){
    //  Check Opcode
    req->opcode = ((uint16_t)buf[0] << 8) | ((uint16_t)buf[1]);
    if(req->opcode != RRQ_OP && req->opcode != WRQ_OP){
        printf("[INFO] Error: Invalid request!\n");
        return ERR_ACCESS_VIOLATE;
    }

    // Check Filename
    if(req->opcode == RRQ_OP){
        strcpy(req->filename, LIB);
        strcat(req->filename, buf + 2);

        if(access(req->filename, F_OK) < 0){
            printf("[INFO] Error: File [%s] not found!\n", req->filename);
            return ERR_FILE_NOT_FOUND;
        }
    }

    if(req->opcode == WRQ_OP){
        strcpy(req->filename, buf + 2);
    }

    // Check Mode
    strcpy(req->mode, buf + 3 + strlen(buf + 2));
    if((strcmp(req->mode, "octet") != 0) && (strcmp(req->mode, "netascii"))){
        printf("[INFO] Error: Invalid access mode: [%s]!\n", req->mode);
        return ERR_ACCESS_VIOLATE;
    }

    return -1;
}

int check_data_recv(unsigned char* buf, char* write_buf, int write_bytes, int global_block_num){
    // Check Opcode
    int opcode = ((uint16_t)buf[0] << 8) | ((uint16_t)buf[1]);
    if(opcode != DATA_OP){
        printf("[Error] Invalid data packet!\n");
        return ERR_ACCESS_VIOLATE;
    }
    // Check block number
    int block_num = ((uint16_t)buf[2] << 8) | ((uint16_t)buf[3]);
    if(block_num != global_block_num){
        printf("[Error] Invalid block number for the incoming data!");
        printf("  Actual : %d ; Expected: %d ;\n", block_num, global_block_num);
        return ERR_ACCESS_VIOLATE;
    }
    // Copy data
    // char* buf_temp = buf + 4;
    memcpy(write_buf, buf + 4, write_bytes);

    return ERR_FREE;
}

int check_ack_recv(unsigned char* buf, uint16_t global_block_num){
    // Check Opcode
    uint16_t opcode = ((uint16_t)buf[0] << 8) | ((uint16_t)buf[1]);
    if(opcode != ACK_OP){
        printf("[Error] Invalid acknowledgement!\n");
        return ERR_ACCESS_VIOLATE;
    }
    // Check block number
    uint16_t block_num = ((uint16_t)buf[2] << 8) | ((uint16_t)buf[3]);
    if(block_num != global_block_num){
        printf("[Error] Invalid block number for acknowledgement!");
        printf("  Actual : %d ; Expected: %d ;\n", block_num, global_block_num);
        return ERR_ACCESS_VIOLATE;
    }

    return ERR_FREE;
}


// > 0 : Descriptor readable
// = 0 : Timeout triggered
// < 0 : Error
int readable_timeo(int fd){
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    return (select(fd + 1, &rset, NULL, NULL, &tv));
}

// Send a data packet and check if the ack packet received properly
int tftp_read_send(char* read_buf, unsigned char* buf, int read_bytes, uint16_t global_block_num, int socket_fd, struct sockaddr_in socket_addr, socklen_t socket_addr_size){
    memset(buf, 0, MAXLINE);
    // Pack TFTP data message
    tftp_data_msg_t data_msg;
    tftp_gen_data_msg(global_block_num, read_buf, &data_msg, read_bytes);
    int size = tftp_pack_data_msg(buf, data_msg, read_bytes);
    // Send TFTP data packet
    if(sendto(socket_fd, buf, size, 0, (struct sockaddr*) &socket_addr, socket_addr_size) < 0){
        perror("[Error]Sendto1 -- ");
        exit(EXIT_FAILURE);
    }
    // Wait for the corresponding ACK packet and check
    int readable = readable_timeo(socket_fd);
    int resend = 0;
    // Timeout triggered
    while(readable == 0){
        if(resend == MAX_RESEND){
            printf("[Error] Reach maximum times of resend. Transfer terminated...\n");
            return -2;
        }
        else{
            ++resend;
            printf("[INFO] Timeout. Resend data packet [%d]...\n", global_block_num);
            if(sendto(socket_fd, buf, size, 0, (struct sockaddr*) &socket_addr, socket_addr_size) < 0){
                perror("[Error]Sendto2 -- ");
                exit(EXIT_FAILURE);
            }
            readable = readable_timeo(socket_fd);
        }
    }

    if(readable < 0){
        perror("[Error]Select -- ");
        exit(EXIT_FAILURE);
    }

    if(readable > 0){
        // printf("[DEBUG] Readable FD detected.\n");
        memset(buf, 0, MAXLINE);
        if(recvfrom(socket_fd, buf, sizeof(tftp_ack_msg_t), 0, (struct sockaddr*) &socket_addr, &socket_addr_size) < 0){
            perror("[Error]Recvfrom (ACK) -- ");
            exit(EXIT_FAILURE);
        }

    int chk_flag = check_ack_recv(buf, global_block_num);
    // printf("[DEBUG] Check Flag : %d\n", chk_flag);
    return chk_flag;
    }
}

// Send a data packet and check if the ack packet received properly
int tftp_write_recv(char* write_buf, unsigned char* buf, int write_bytes, uint16_t global_block_num, int socket_fd, struct sockaddr_in socket_addr, socklen_t socket_addr_size){
    int chk_flag = check_data_recv(buf, write_buf, write_bytes, global_block_num);
    // printf("[DEBUG] Check Flag: %d\n", chk_flag);
    if(chk_flag < 0){
        // Send Ack
        memset(buf, 0, MAXLINE);
        tftp_pack_ack_msg(buf, global_block_num);
        if(sendto(socket_fd, buf, 4, 0, (struct sockaddr*) &socket_addr, socket_addr_size) < 0){
            perror("[Error]Sendto(ack) -- ");
            exit(EXIT_FAILURE);
        }
    }
    return chk_flag;
}

void tftp_read_octet(FILE** fp, unsigned char* buffer, int socket_fd, struct sockaddr_in socket_addr, socklen_t socket_addr_size){
    int read_bytes;
    unsigned char read_buf[BLOCKSIZE];
    uint16_t global_block_num = 1;
    int err_flag;

    while(!feof(*fp)){
        memset(read_buf, 0, BLOCKSIZE);
        read_bytes = fread(read_buf, 1, BLOCKSIZE, *fp);
        err_flag = tftp_read_send(read_buf, buffer, read_bytes, global_block_num, socket_fd, socket_addr, socket_addr_size);
        if(err_flag > 0){
            printf("[Error] Ack Error: %d\n", err_flag);
            tftp_error_msg_t err_msg;
            tftp_gen_error_msg(err_flag, "", &err_msg);

            int size = tftp_pack_error_msg(buffer, err_msg);
            if(sendto(socket_fd, buffer, size, 0, (struct sockaddr*) &socket_addr, socket_addr_size) < 0){
                perror("[Error]Sendto(octet) -- ");
                exit(EXIT_FAILURE);
            }
        }
        else if(err_flag == 0){
        // TODO: Send error packet with not defined type
        }
        else if(err_flag == -1){
            // Wrap-around
            if(global_block_num == 65535)
                global_block_num = 0;
            else
                global_block_num++;
        }
        else{
            close(socket_fd);
            exit(EXIT_FAILURE);
        }
    }
}

void tftp_write_octet(FILE** fp, unsigned char* buffer, int socket_fd, struct sockaddr_in socket_addr, socklen_t socket_addr_size){
    int write_bytes = BLOCKSIZE;
    unsigned char write_buf[BLOCKSIZE];
    uint16_t global_block_num = 1;
    int err_flag;

    memset(buffer, 0, MAXLINE);
    tftp_pack_ack_msg(buffer, 0);
    if(sendto(socket_fd, buffer, 4, 0, (struct sockaddr*) &socket_addr, socket_addr_size) < 0){
        perror("[Error]Sendto(ack0) -- ");
        exit(EXIT_FAILURE);
    }

    while(write_bytes == BLOCKSIZE){
        // Get received data
        memset(buffer, 0, MAXLINE);
        write_bytes = recvfrom(socket_fd, buffer, sizeof(tftp_data_msg_t), 0, (struct sockaddr*) &socket_addr, &socket_addr_size) - 4;
        if(write_bytes < 0){
            perror("[Error]Recvfrom (DATA) -- ");
            exit(EXIT_FAILURE);
        }
        memset(write_buf, 0, BLOCKSIZE);
        // printf("[DEBUG] Bytes read from the buffer: %d\n", write_bytes);
        // printf("[DEBUG] (READ) First C : %d ; Last C : %d ;\n", read_buf[0], read_buf[read_bytes - 1]);
        err_flag = tftp_write_recv(write_buf, buffer, write_bytes, global_block_num, socket_fd, socket_addr, socket_addr_size);
        // printf("[DEBUG] Error Flag: %d\n", err_flag);

        if(err_flag > 0){
            printf("[Error] Data Error: %d\n", err_flag);
            tftp_error_msg_t err_msg;
            tftp_gen_error_msg(err_flag, "", &err_msg);

            int size = tftp_pack_error_msg(buffer, err_msg);
            if(sendto(socket_fd, buffer, size, 0, (struct sockaddr*) &socket_addr, socket_addr_size) < 0){
                perror("[Error]Sendto(octet) -- ");
                exit(EXIT_FAILURE);
            }
        }
        else if(err_flag == 0){
        // TODO: Send error packet with not defined type
        }
        else if(err_flag == -1){
            fseek(*fp, 0, SEEK_END);
            (void)fwrite(write_buf, 1, write_bytes, *fp);
            // Wrap-around
            if(global_block_num == 65535)
                global_block_num = 0;
            else
                global_block_num++;
        }
        else{
            close(socket_fd);
            exit(EXIT_FAILURE);
        }
    }
}

// Read with NETASCII mode
void tftp_read_netascii(FILE** fp, char* buffer, int socket_fd, struct sockaddr_in socket_addr, socklen_t socket_addr_size){
    FILE *fileptr;
    char temp_file[] = "temp_file.txt"; //for storing file with CR and LF
    char read_data[BLOCKSIZE];
    int read_bytes;
    int c=1,j,err_flag;
    uint16_t global_block_num=1;  //blocknumber

    if((fileptr = fopen(temp_file,"w")) == NULL){
      perror("[Error] File not found in the vault");
      exit(0);
    }
    //reading each char
    while(c != EOF){
        c = fgetc(*fp);
        if(c == '\n'){   //LF -> CR LF
            fputc('\r', fileptr);
            fputc(c, fileptr);
        }
        else if(c == '\r'){  //CR -> CR NULL
            fputc(c, fileptr);
            fputc('\0', fileptr);
        }
        else if(c == EOF){
            break;
        }
        else{
            fputc(c, fileptr); //copy to a temp file
        }
    }
    fclose(fileptr);

    if((fileptr=fopen(temp_file,"r"))==NULL){
        perror("Open Error");
        exit(0);
    }

    else{
        fseek(fileptr , 0, SEEK_SET);
        while(!feof(fileptr)){
            read_bytes = fread(read_data, 1, BLOCKSIZE, fileptr);
            read_data[read_bytes]='\0';
            err_flag = tftp_read_send(read_data, buffer, read_bytes, global_block_num, socket_fd, socket_addr, socket_addr_size);
            if(err_flag > 0){
                printf("[Error] Ack Error: %d\n", err_flag);
                tftp_error_msg_t err_msg;
                tftp_gen_error_msg(err_flag, "", &err_msg);

                int size = tftp_pack_error_msg(buffer, err_msg);
                if(sendto(socket_fd, buffer, size, 0, (struct sockaddr*) &socket_addr, socket_addr_size) < 0){
                    perror("[Error]Sendto(netascii) -- ");
                    exit(EXIT_FAILURE);
                }
            }
			else if(err_flag == 0){
				// TODO: Send error packet with not defined type
			}
            else if(err_flag == -1){
                // Wrap-around
                if(global_block_num == 65535)
                    global_block_num = 0;
                else
                    global_block_num++;
            }
            else{
                close(socket_fd);
                exit(EXIT_FAILURE);
            }
        }
    }
    fclose(fileptr);
}

// TFTP read transaction. Called in tftp_transfer.
int tftp_read(char* filename, char* mode, char* buffer, int socket_fd, struct sockaddr_in socket_addr, socklen_t socket_addr_size){

    printf("[INFO] File Name : %s\n", filename);
    printf("[INFO] Mode : %s\n\n", mode);

    struct timeval tv;
    FILE *fp;

    //Check if file is being read
    if((fp = fopen(filename, "r")) == NULL){
        printf("[Error] The file is not readable! \n");
        return -1;
    }
    else{
        fseek(fp, 0, SEEK_SET);
        printf("[INFO] File [%s] read successfully\n", filename);
		//check mode
        if(strcmp(mode, "octet") == 0){
            
            tftp_read_octet(&fp, buffer, socket_fd, socket_addr, socket_addr_size);
            printf("[INFO] RRQ to file [%s] from port [%d] completes!\n", filename, socket_fd);
            return 1;
        }
        else if(strcmp(mode, "netascii") == 0){
            tftp_read_netascii(&fp, buffer, socket_fd, socket_addr, socket_addr_size);
            printf("[INFO] RRQ to file [%s] from port [%d] completes!\n", filename, socket_fd);
            return 1;
        }
        else{
            printf("[Error] Invalid Mode!\n");
            return -1;
        }
    }
    fclose(fp);
}

// TFTP read transaction. Called in tftp_transfer.
int tftp_write(char* filename, char* mode, char* buffer, int socket_fd, struct sockaddr_in socket_addr, socklen_t socket_addr_size){

    printf("[INFO] File Name : %s\n", filename);
    printf("[INFO] Mode : %s\n\n", mode);

    FILE *fp;

    // Open a file to be written
    char full_name[64];
    sprintf(full_name, "%s%s", LIB, filename);
    if((fp = fopen(full_name, "w+")) == NULL){
        printf("[Error] Cannot create a new file! \n");
        return -1;
    }
    else{
        fseek(fp, 0, SEEK_SET);
        printf("[INFO] File [%s] created.\n", full_name);
        if(strcmp(mode, "octet") == 0){
            printf("[INFO] Operation Mode: OCTET\n\n");
            tftp_write_octet(&fp, buffer, socket_fd, socket_addr, socket_addr_size);
            printf("[INFO] WRQ to file [%s] from port [%d] completes!\n", filename, socket_fd);
            return 1;
        }
        else if(strcmp(mode, "netascii") == 0){
            // printf("[INFO] Operation Mode: NETASCII\n\n");
            // tftp_read_netascii(&fp, buffer, socket_fd, socket_addr, socket_addr_size);
            // printf("[INFO] RRQ to file [%s] from port [%d] completes!\n", filename, socket_fd);
            // return 1;
        }
        else{
            printf("[Error] Invalid Mode!\n");
            return -1;
        }
    }
    fclose(fp);
}

// The function to handle tftp transfer
int tftp_transfer(tftp_req_msg_t req, unsigned char* buffer, struct sockaddr_in socket_addr, socklen_t socket_addr_size, struct sockaddr_in serv_addr){
    int socket_fd;

    printf("\n[INFO] Op Code : %d\n", req.opcode);

    // Open an ephemeral port for new requests
    if((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("[Error]Socket(Child) -- ");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_port = htons(0);
    if(bind(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("[Error]Binding -- ");
        exit(EXIT_FAILURE);
    }

    // Handle read requests
    if(req.opcode == RRQ_OP){
        printf("[INFO] Received a READ request from port [%d].\n", socket_fd);
        if(tftp_read(req.filename, req.mode, buffer, socket_fd, socket_addr, socket_addr_size) > 0){
            return 1;
        }
    }
    // Handle write requests
    if(req.opcode == WRQ_OP){
        printf("[INFO] Received a WRITE request from port [%d].\n", socket_fd);
        if(tftp_write(req.filename, req.mode, buffer, socket_fd, socket_addr, socket_addr_size) > 0){
            return 1;
        }
    }

    return -1;
}


