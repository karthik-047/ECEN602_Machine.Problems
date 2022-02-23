#ifndef SBCP_PROTOCOL
#define SBCP_PROTOCOL

#include <stdint.h>
#include <stdlib.h>

// SBCP Protocol Version
#define SBCP_VRSN 3
// SBCP Message Type
#define MSG_JOIN    2
#define MSG_ACK     7
#define MSG_NAK     5
#define MSG_FWD     3
#define MSG_SEND    4
#define MSG_ONLINE  8
#define MSG_OFFLINE 6
#define MSG_IDLE    9
// SBCP Attribute Type
#define ATTR_USERNAME   2
#define ATTR_MESSAGE    4
#define ATTR_REASON     1
#define ATTR_COUNTC     3

struct sbcp_message{
    uint16_t    vrsn;
    uint8_t     type;
    uint16_t    length;
    struct sbcp_attribute* msg_attribute;
};

struct sbcp_attribute{
    uint16_t    type;
    uint16_t    length;
    char*       payload;
};


unsigned char* field_concat(const unsigned char*, const unsigned char*, const char*);
unsigned char* sbcp_pack(struct sbcp_message*);
void sbcp_unpack(unsigned char*, struct sbcp_message*);
void sbcp_print_dbg_pre_pack(struct sbcp_message*);
void sbcp_print_dbg_post_pack(unsigned char*);
void sbcp_print_dbg_header(unsigned char*);
char* sbcp_pkt_tcp_segmentation(unsigned char*, struct sbcp_message*);

#endif
