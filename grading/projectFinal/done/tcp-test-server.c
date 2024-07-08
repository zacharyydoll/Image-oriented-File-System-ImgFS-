#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socket_layer.h"
#include "error.h"

#define MAX 2048
#define MAX_FILE_SIZE 1024 // Max file size in bytes
#define ACK "ACK" // Acknowledgment message
#define NACK "NACK" // Negative acknowledgment message

/**
 * Receives a file sent over a TCP connection.
 * Source file is received in chunks and written to a local file.
 *
 * @param connfd File descriptor for the active TCP connection.
 */
void receive_file(int connfd)
{
    // Buffer to store received data chunk.
    char buffer[MAX];
    FILE *file;

    // Initializing the buffer
    bzero(buffer, MAX);

    // Reading the size of the incoming file from the connection
    size_t len ;
    size_t file_size = atoll(buffer);

    //Resetting the buffer
    bzero(buffer, MAX);




    if (file_size != 0) {
        printf("Received a size: %ld ", file_size);

        if (file_size > MAX_FILE_SIZE) {
            printf("--> rejected\n");
            tcp_send(connfd, NACK, strlen(NACK) + 1);

        }
        tcp_send(connfd, ACK, strlen(ACK) + 1);
        printf("--> accepted\nAbout to receive file of %zu bytes\n", file_size);


        file = fopen("received_file", "w");
        if (file == NULL) {
            perror("Failed to create a file");

        }

        // Receiving the data in chunks and writing it to the file
        size_t received_bytes = 0;
        while (received_bytes < file_size) {
            len = tcp_read(connfd, buffer, MAX);
            received_bytes += len;

            fwrite(buffer, sizeof(char), len, file);
            bzero(buffer, MAX);
        }

        fclose(file);

        printf("Received a file:\n");

        //Displaying the received file
        system("cat received_file");

        // Sending an acknowledgement after receiving and writing the entire file
        tcp_send(connfd, ACK, strlen(ACK) + 1);


    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        return ERR_INVALID_ARGUMENT;
    }
    int sockfd, connfd;
    int port ;
    sscanf(argv[1], "%d", &port);

    sockfd = tcp_server_init(port);
    if (sockfd <0) {
        perror("Socket creation failed");
        exit(0);
    }
    printf("Server started on port %i \n",port);

    while(1) {
        printf("Waiting for a size...\n");
        connfd = tcp_accept(sockfd);
        if (connfd < 0) {
            perror("Server accept failed");
            exit(EXIT_FAILURE);
        }
        receive_file(connfd);
    }

}

