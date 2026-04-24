#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet)
{
    uint16_t opcode = ntohs(packet->opcode); // Converts network → host format

    if (opcode == RRQ) // Client wants to download file
    {
        printf("RRQ request for file: %s\n", packet->body.request.filename);

        send_file(sockfd, client_addr, client_len, packet->body.request.filename); // Call function to send file to client
    }

    else if (opcode == WRQ) // Client wants to upload file
    {
        printf("WRQ request for file: %s\n", packet->body.request.filename);

        receive_file(sockfd, client_addr, client_len, packet->body.request.filename); // Receive file from client
    }
}

int main()
{
    int sockfd; // Socket file descriptor

    struct sockaddr_in server_addr, client_addr; // Store: server address, client address

    socklen_t client_len = sizeof(client_addr); // Size of client address

    tftp_packet packet; // Structure to store incoming data

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Creates UDP socket , AF_INET → IPv4, SOCK_DGRAM → UDP

    struct timeval tv; // Set timeout value (seconds)
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); // If no data comes: recvfrom() will not wait forever, It returns error after timeout

    memset(&server_addr, 0, sizeof(server_addr)); // Make everything 0

    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_port = htons(PORT);       // Port Number
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept from any IP

    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)); // Server listens on port 6969

    printf("TFTP Server listening on port %d...\n\n", PORT);

    while (1)
    {
        int n = recvfrom(sockfd, &packet, sizeof(packet), 0, // Waits for client request
                         (struct sockaddr *)&client_addr,    // data → packet, client info → client_addr
                         &client_len);

        if (n < 0)
            continue;

        handle_client(sockfd, client_addr, client_len, &packet); // Decides:RRQ → send file, WRQ → receive file
    }

    close(sockfd);

    return 0;
}
