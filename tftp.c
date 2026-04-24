#include "tftp.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)
{
    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        printf("File not found\n");
        return;
    }

    tftp_packet packet;
    tftp_packet ack;

    int block = 1;
    int n;

    while ((n = read(fd, packet.body.data_packet.data, BUFFER_SIZE)) > 0) // Read 512 bytes
    {
        packet.opcode = htons(DATA); // Set DATA packet
        packet.body.data_packet.block_number = htons(block);

        printf("Sending block %d with %d bytes\n", block, n);

        sendto(sockfd, &packet, n + 4, 0,
               (struct sockaddr *)&client_addr, client_len); // Send to client

        recvfrom(sockfd, &ack, sizeof(ack), 0,
                 (struct sockaddr *)&client_addr, &client_len); // Wait for ACK

        printf("ACK received for block %d\n", block);

        block++; // Next block

        if (n < BUFFER_SIZE) // Last packet
            break;
    }

    close(fd);

    printf("File sent successfully\n");
}

void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)
{
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);

    if (fd < 0)
    {
        printf("File create error\n");
        return;
    }

    tftp_packet packet;
    tftp_packet ack;

    int n;
    ack.opcode = htons(ACK);
    ack.body.ack_packet.block_number = htons(0); // TFTP rule: start with ACK 0

    sendto(sockfd, &ack, sizeof(ack), 0, // SEnd the ack
           (struct sockaddr *)&client_addr, client_len);

    while (1)
    {
        n = recvfrom(sockfd, &packet, sizeof(packet), 0, // Receive data
                     (struct sockaddr *)&client_addr, &client_len);

        int data_size = n - 4;

        write(fd, packet.body.data_packet.data, data_size); // Save file

        ack.opcode = htons(ACK);
        ack.body.ack_packet.block_number =
            packet.body.data_packet.block_number;

        sendto(sockfd, &ack, sizeof(ack), 0,                 // send ack
               (struct sockaddr *)&client_addr, client_len); // Confirm

        if (data_size < BUFFER_SIZE)
            break;
    }

    close(fd);

    printf("File received successfully\n\n");
}
