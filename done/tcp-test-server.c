#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "socket_layer.h"
#include "error.h"

#define BUFFER_SIZE 1024
#define DELIMITER '\0'
#define EOF_DELIMITER "<EOF>"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        return ERR_INVALID_ARGUMENT;
    }
    uint16_t server_port_num = atoi(argv[1]);
    int listen_fd = tcp_server_init(server_port_num);

    printf("Server started on port %i\n",server_port_num);

    while (1) {
        printf("Waiting for a size...\n");
        int client_fd = tcp_accept(listen_fd);

        //char buffer[BUFFER_SIZE];
        uint32_t file_size;

        if (tcp_read(client_fd, (char*)&file_size, sizeof(file_size)) != sizeof(file_size)) {
            fprintf(stderr, "ERROR: Failed to read size.\n");
            continue;
        }

        file_size = ntohl(file_size);
        printf("Received a size: %u ", file_size);
        if (file_size > 1024) {
            printf("--> rejected\n");
            tcp_send(client_fd, "ACK SIZE TOO LARGE", 18);
            continue;
        }
        printf("--> accepted\nAbout to receive file of %u bytes\n", file_size);
        char buffer[1024 + 1];
        memset(buffer, 0, sizeof(buffer));  // Initialize buffer with zeros

        if (tcp_read(client_fd, buffer, file_size) != file_size) {
            fprintf(stderr, "ERROR: Failed to read file.\n");
            continue;
        }

        printf("Received a file:\n%s\n\n", buffer);

        const char* response = (file_size <= 1024) ? "ACK SIZE OK" : "ACK SIZE TOO LARGE";
        tcp_send(client_fd, "ACK FILE RECEIVED", strlen("ACK FILE RECEIVED"));
        tcp_send(client_fd, DELIMITER, sizeof(DELIMITER));  // Send delimiter


    }

    return 0;
}
