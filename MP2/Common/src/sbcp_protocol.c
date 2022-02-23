#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "sbcp_protocol.h"

// Concatenate the Message header, Attribute header and Payload to form the packet to send
// Length of Message header:   4
// Length of Attribute header: 4
// Length of Message:          1-152
unsigned char* field_concat(const unsigned char* msg_header, const unsigned char* attr_header, const char* payload)
{
    const size_t payload_len = strlen(payload);

    unsigned char* packet = malloc(8 + payload_len + 1); // +1 for NULL

    memcpy(packet, msg_header, 4);
    memcpy(packet + 4, attr_header, 4);
    memcpy(packet + 8, payload, payload_len + 1); // +1 for NULL

    return packet;
}

// Pack the sbcp_message structure into a string for writing
unsigned char* sbcp_pack(struct sbcp_message* send_packet) {

    uint32_t msg_header = (send_packet->vrsn << 23) + (send_packet->type << 16) + send_packet->length;
    uint32_t attr_header = (send_packet->msg_attribute->type << 16) + send_packet->msg_attribute->length;

    unsigned char msg_header_c[4] = {(msg_header & 0xFF000000) >> 24, (msg_header & 0x00FF0000) >> 16, (msg_header & 0x0000FF00) >> 8, (msg_header & 0x000000FF)};
    unsigned char attr_header_c[4] = { (attr_header & 0xFF000000) >> 24, (attr_header & 0x00FF0000) >> 16, (attr_header & 0x0000FF00) >> 8, (attr_header & 0x000000FF) };

    unsigned char* packet = field_concat(msg_header_c, attr_header_c, send_packet->msg_attribute->payload);

    return packet;

}

// Unpack the string into sbcp_message structure
void sbcp_unpack(unsigned char* packet, struct sbcp_message* recv_msg){
    unsigned char msg_header_c_recv[4];
    unsigned char attr_header_c_recv[4];
    memcpy(msg_header_c_recv, packet, 4);
    memcpy(attr_header_c_recv, packet + 4, 4);

    recv_msg->type = msg_header_c_recv[1] & 0x7F;
    recv_msg->vrsn = ((msg_header_c_recv[0] << 8) + (msg_header_c_recv[1] & 0x80)) >> 7;
    recv_msg->length = (msg_header_c_recv[2] << 8) + msg_header_c_recv[3];

    recv_msg->msg_attribute->type = (attr_header_c_recv[0] << 8) + attr_header_c_recv[1];
    recv_msg->msg_attribute->length = (attr_header_c_recv[2] << 8) + attr_header_c_recv[3];
    recv_msg->msg_attribute->payload = packet + 8;
}

char* sbcp_pkt_tcp_segmentation(unsigned char* buff, struct sbcp_message* msg) {
    unsigned char buff_header[4];
    memcpy(buff_header, buff, 4);

    int pkt_length = (buff_header[2] << 8) + buff_header[3];

    unsigned char* single_pkt = malloc(pkt_length + 1);
    memcpy(single_pkt, buff, pkt_length);
    single_pkt[pkt_length] = '\0';

    struct sbcp_message *rmsg_p, rmsg;
    struct sbcp_attribute rattr;
    rmsg_p = &rmsg;
    rmsg.msg_attribute = &rattr;
    sbcp_unpack(single_pkt, rmsg_p);

    msg->type = rmsg.type;
    msg->vrsn = rmsg.vrsn;
    msg->length = rmsg.length;
    msg->msg_attribute->type = rattr.type;
    msg->msg_attribute->length = rattr.length;
    strcpy(msg->msg_attribute->payload, single_pkt + 8);

    unsigned char next_flag_c[2];
    memcpy(next_flag_c, buff + pkt_length, 2);
    int next_flag = ((next_flag_c[0] << 8) + (next_flag_c[1] & 0x80)) >> 7;

    if (next_flag == 3) {
        // printf("\nNext packet exists.\n");
        char* buff_new = buff + pkt_length;
        free(single_pkt);
        return buff_new;
    }
    else {
        // printf("\nLast packet.\n");
        free(single_pkt);
        return NULL;
    }
}

// For SBCP debug
void sbcp_print_dbg_pre_pack(struct sbcp_message* packet){
    printf("\n*****DEBUG PRE-PACK******\n");
    printf("Message Version:   %d\n", packet->vrsn);
    printf("Message Type:      %d\n", packet->type);
    printf("Message Length:    %d\n", packet->length);
    printf("Attribute Type:    %d\n", packet->msg_attribute->type);
    printf("Attribute Length:  %d\n", packet->msg_attribute->length);
    printf("Attribute Payload: %s\n", packet->msg_attribute->payload);
    printf("*************************\n\n");
}


void sbcp_print_dbg_post_pack(unsigned char* packet){
    struct sbcp_message *recv_msg, rmsg;
    struct sbcp_attribute rattr;
    recv_msg = &rmsg;
    rmsg.msg_attribute = &rattr;

    sbcp_unpack(packet, recv_msg);
    printf("\n*****DEBUG POST-PACK*****\n");
    printf("Message Version:   %d\n", rmsg.vrsn);
    printf("Message Type:      %d\n", rmsg.type);
    printf("Message Length:    %d\n", rmsg.length);
    printf("Attribute Type:    %d\n", rattr.type);
    printf("Attribute Length:  %d\n", rattr.length);
    printf("Attribute Payload: %s\n", rattr.payload);
    printf("*************************\n\n");
}

void sbcp_print_dbg_header(unsigned char* packet){
    printf("\n*****DEBUG RAW PAKET*****\n");
    printf("Length of Pkt: %ld\n", sizeof(packet));
    printf("[0]: %x\n", packet[0]);
    printf("[1]: %x\n", packet[1]);
    printf("[2]: %x\n", packet[2]);
    printf("[3]: %x\n", packet[3]);
    printf("[4]: %x\n", packet[4]);
    printf("[5]: %x\n", packet[5]);
    printf("[6]: %x\n", packet[6]);
    printf("[7]: %x\n", packet[7]);
    printf("*************************\n\n");
}
