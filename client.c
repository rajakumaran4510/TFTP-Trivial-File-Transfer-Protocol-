#include "tftp.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

int main()
{
    char command[256];                  // user input
    tftp_client_t client;               // stores socket + server info
    memset(&client, 0, sizeof(client)); // initialize to 0

    while (1)
    {
        printf("\nType->connect\nType->put\nType->get\nType->bye\n\n");
        printf("TFTP: ");
        fgets(command, sizeof(command), stdin); // Read user input

        command[strcspn(command, "\n")] = 0;

        process_command(&client, command); // Execute command
    }

    return 0;
}

void process_command(tftp_client_t *client, char *command)
{
    if (strcmp(command, "connect") == 0)
    {
        printf("Enter the IP address: ");
        scanf("%s", client->server_ip); // User enters IP
        getchar();

        if (validate_ip(client->server_ip) == 0) // Checks IP format
        {
            printf("Invalid address\n");
            return;
        }

        printf("Address is valid\n");
        connect_to_server(client, client->server_ip, PORT); // Create socket + store server details
        return;
    }

    if (strcmp(command, "put") == 0)
    {
        char filename[20];
        printf("Enter the filename: ");
        scanf("%s", filename);
        getchar();

        put_file(client, filename); // Send file → client ➜ server
        return;
    }

    if (strcmp(command, "get") == 0)
    {
        char filename[20];
        printf("Enter the filename: ");
        scanf("%s", filename);
        getchar();

        get_file(client, filename); // Receive file → server ➜ client
        return;
    }

    if (strcmp(command, "bye") == 0)
    {
        disconnect(client);
        exit(0);
    }
}

void connect_to_server(tftp_client_t *client, char *ip, int port)
{
    client->sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Create UDP socket

    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(PORT); // Convert IP string → binary

    if (inet_pton(AF_INET, ip, &client->server_addr.sin_addr) <= 0)
    {
        printf("Invalid IP\n");
        return;
    }

    client->server_len = sizeof(client->server_addr);

    printf("Connected to server %s:%d\n", ip, port);
}

void put_file(tftp_client_t *client, char *filename)
{
    int fd = open(filename, O_RDONLY);

    if (fd == -1)
    {
        printf("Please provide valid file\n");
        return;
    }

    send_request(client->sockfd, client->server_addr, filename, WRQ); // Send WRITE REQUEST to server

    tftp_packet packet;
    tftp_packet ack;

    int block = 1;
    int n;

    while ((n = read(fd, packet.body.data_packet.data, BUFFER_SIZE)) > 0) // Read 512 bytes
    {
        printf("Sending block %d with %d bytes\n", block, n);
        packet.opcode = htons(DATA);
        packet.body.data_packet.block_number = htons(block);

        sendto(client->sockfd, &packet, n + 4, 0, // Send data to server
               (struct sockaddr *)&client->server_addr,
               client->server_len);

        recvfrom(client->sockfd, &ack, sizeof(ack), 0, // Wait for ACK
                 (struct sockaddr *)&client->server_addr,
                 &client->server_len);
        printf("ACK received for block %d\n", block);
        block++;

        if (n < BUFFER_SIZE) // Last block → stop
            break;
    }

    close(fd);

    printf("File uploaded successfully\n");
}

void get_file(tftp_client_t *client, char *filename)
{
    send_request(client->sockfd, client->server_addr, filename, RRQ); // Ask server for file

    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);

    if (fd < 0)
    {
        printf("File create error\n");
        return;
    }

    tftp_packet packet;
    tftp_packet ack;

    while (1)
    {
        memset(&packet, 0, sizeof(packet));
        int n = recvfrom(client->sockfd, &packet, sizeof(packet), 0, // Receive DATA
                         (struct sockaddr *)&client->server_addr,
                         &client->server_len);

        if (n < 4)
            continue;

        if (ntohs(packet.opcode) != DATA) // Ignore wrong packet
            continue;

        int block = ntohs(packet.body.data_packet.block_number); // Get block number

        int data_size = n - 4;

        printf("Received block %d with %d bytes\n", block, data_size);

        write(fd, packet.body.data_packet.data, data_size);

        ack.opcode = htons(ACK); // Prepare ACK
        ack.body.ack_packet.block_number = htons(block);

        sendto(client->sockfd, &ack, sizeof(ack), 0, // Send ACK
               (struct sockaddr *)&client->server_addr,
               client->server_len);

        printf("ACK sent for block %d\n", block);

        // if (data_size < BUFFER_SIZE)
        break;
    }

    close(fd);

    printf("File downloaded successfully\n");
}

void disconnect(tftp_client_t *client)
{
    close(client->sockfd);

    printf("Disconnected from server\n");
}

void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    char buffer[516];
    socklen_t len = sizeof(server_addr);

    buffer[0] = 0;
    buffer[1] = opcode; // Set request type

    strcpy(&buffer[2], filename);                       // Add filename
    strcpy(&buffer[2 + strlen(filename) + 1], "octet"); // Mode = binary

    sendto(sockfd, buffer, // Send request to server
           2 + strlen(filename) + 1 + strlen("octet") + 1,
           0,
           (struct sockaddr *)&server_addr,
           len);
}

int validate_ip(char *ip_add)
{
    struct sockaddr_in addr;

    if (inet_pton(AF_INET, ip_add, &addr.sin_addr) <= 0)
        return 0;

    return 1;
}
