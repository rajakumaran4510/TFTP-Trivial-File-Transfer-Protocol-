#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <netinet/in.h>

#define PORT 6969
#define BUFFER_SIZE 512 //TFTP sends data in 512-byte blocks
#define TIMEOUT_SEC 5

#define RRQ 1 //Read Request
#define WRQ 2 //Write Request
#define DATA 3
#define ACK 4
#define ERROR 5

//Packet structure
typedef struct
{
    uint16_t opcode;

    union
    {
        struct
        {
            uint16_t block_number; //Sends file in chunks
            char data[BUFFER_SIZE];
        } data_packet;

        struct
        {
            uint16_t block_number; //Confirms received block
        } ack_packet;

        struct
        {
            char filename[100]; //Used in get / put
            char mode[10];
        } request;

    } body;

} tftp_packet;

void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);
void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);

#endif
