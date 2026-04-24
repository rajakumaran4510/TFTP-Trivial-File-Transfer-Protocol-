#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include <netinet/in.h>

typedef struct
{
    int sockfd;
    char server_ip[50];
    struct sockaddr_in server_addr;
    socklen_t server_len;

} tftp_client_t;

void process_command(tftp_client_t *client, char *command);
void connect_to_server(tftp_client_t *client, char *ip, int port);
void put_file(tftp_client_t *client, char *filename);
void get_file(tftp_client_t *client, char *filename);
void disconnect(tftp_client_t *client);

void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode);

int validate_ip(char *ip_add);

#endif
