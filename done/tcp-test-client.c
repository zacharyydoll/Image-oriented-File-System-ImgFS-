#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "socket_layer.h"
#include "error.h"


#define BUFFER_SIZE 2048
#define DELIMITER '\0'
#define EOF_DELIMITER "<EOF>"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s port file\n", argv[0]);
        return ERR_INVALID_ARGUMENT;
    }

    uint16_t server_port_num = atoi(argv[1]);

    char* file_path = argv[2];
    printf("Talking to  %li\n", server_port_num);

    FILE* fp = fopen(file_path, "r");
    if (fp == NULL) {
        perror("Error opening file");
        return ERR_IO;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (filesize > 2048) {
        fprintf(stderr, "ERROR: File too large.\n");
        return ERR_INVALID_ARGUMENT;
    }

    printf("Sending size %li\n", filesize);

    int socket_fd = tcp_server_init(server_port_num);
    uint32_t network_byte_order_filesize = htonl(filesize);
    tcp_send(socket_fd, (char*) &network_byte_order_filesize, sizeof(network_byte_order_filesize));
    tcp_send(socket_fd, DELIMITER, sizeof(DELIMITER));  // Send delimiter

    char response[2048+1];
    ssize_t size_response_len = tcp_read(socket_fd, response, 2048);
    if (tcp_read(socket_fd, response, sizeof(response)) < 0) {
        return ERR_INVALID_ARGUMENT;
    }
    response[size_response_len] = '\0';
    printf("Server responded: %s\n", response);

    if (strcmp(response, "ACK SIZE TOO LARGE")==0) {
        printf("Server rejected due to large file size\n");
        fclose(fp);
        close(socket_fd);
        return 0;
    }
    printf("Sending %s\n", file_path);
    char file_buffer[2048 + 1];

    fread(file_buffer, 1, filesize, fp);
    tcp_send(socket_fd, file_buffer, filesize);

    ssize_t file_response_len = tcp_read(socket_fd, response, 2048);
    response[file_response_len] = '\0';

    if (strcmp(response, "ACK FILE RECEIVED")==0){
        printf("Accepted\nDone\n");
    }

    fclose(fp);
    close(socket_fd);

    return 0;
}
