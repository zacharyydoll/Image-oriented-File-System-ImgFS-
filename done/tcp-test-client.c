
#include <arpa/inet.h> // inet_addr()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include "socket_layer.h"

#define MAX 2048
#define MAX_FILE_SIZE 1024
#define SA struct sockaddr
#define ACK "ACK"

/**
 * Sends a file over a socket
 *
 * @param sockfd Is the file descriptor of the socket on which to send the file
 * @param file_path Is the path to the file to be sent.
 */
void send_file(int sockfd, char *file_path)
{

    //Buffer for storing file data for sending over the socket
    char buffer[MAX];

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error in opening file");
        return ;
    }

    //Seeking to the end of the file to determine the file size
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);

    //Rewinding to move to start of the file again
    rewind(file);

    if (file_size > MAX_FILE_SIZE) {
        fclose(file);
        perror("File size is big");
        return ;
    }

    printf("Sending size %lu:\n",file_size);
    sprintf(buffer, "%ld", file_size);
    write(sockfd, buffer, MAX);

    bzero(buffer, MAX);

    //Reading the server response from the socket
    tcp_read(sockfd, buffer, MAX);

    if (strcmp(buffer, ACK) != 0) {
        fclose(file);
        perror("Acknowledgment not received");
        return ;
    }
    printf("Server responded: \"Small file\" \n");
    bzero(buffer, MAX);

    printf("Sending %s: \n",file_path);
    while (fread(buffer, 1, MAX, file) > 0) {
        write(sockfd, buffer, MAX);
        bzero(buffer, MAX);
    }

    tcp_read(sockfd, buffer, MAX);

    if (strcmp(buffer, ACK) != 0) {
        fclose(file);
        perror("Refused\n");
        return ;
    }
    printf("Accepted \n");

    fclose(file);
}

int main(int argc, char *argv[])
{

    //Argument validity check
    if (argc != 3) {
        printf("Usage: %s <port> <file>\n", argv[0]);
        exit(0);
    }

    int sockfd, port;
    struct sockaddr_in servaddr;
    sscanf(argv[1], "%d", &port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // socket creation and verification
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }

    //Initializing socket for connection
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // Connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        perror("Connection with the server failed");
        exit(0);
    }

    printf("Talking to %i \n",port);

    // Sending the file to the server
    send_file(sockfd, argv[2]) ;

    printf("Done \n");

    //Closing the socket
    close(sockfd);
}
