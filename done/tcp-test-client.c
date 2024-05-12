
#include <arpa/inet.h> // inet_addr()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#define MAX 2048
#define PORT 8080
#define MAX_FILE_SIZE 1024
#define SA struct sockaddr
#define ACK "ACK"
int send_file(int sockfd, char *file_path)
{
    char buffer[MAX];


    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        perror("Error in opening file");
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    if (file_size > MAX_FILE_SIZE)
    {
        fclose(file);
        perror("File size is big");
        return -1;
    }
    printf("Sending size %lu:\n",file_size);
    sprintf(buffer, "%ld", file_size);
    write(sockfd, buffer, MAX);

    bzero(buffer, MAX);
    read(sockfd, buffer, MAX);
    if (strcmp(buffer, ACK) != 0)
    {
        fclose(file);
        perror("Acknowledgment not received");
        return -1;
    }
    printf("Server responded: \"Small file\" \n");
    bzero(buffer, MAX);

    printf("Sending %s: \n",file_path);
    while (fread(buffer, 1, MAX, file) > 0)
    {
        write(sockfd, buffer, MAX);
        bzero(buffer, MAX);
    }

    read(sockfd, buffer, MAX);
    if (strcmp(buffer, ACK) != 0)
    {
        fclose(file);
        perror("Refused\n");
        return -1;
    }
    printf("Accepted \n");

    fclose(file);
}

int main(int argc, char *argv[]){
    if (argc != 3)
    {
        printf("Usage: %s <port> <file>\n", argv[0]);
        exit(0);
    }

    int sockfd, port;
    struct sockaddr_in servaddr;

    sscanf(argv[1], "%d", &port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // socket create and verification
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        perror("Connection with the server failed");
        exit(0);
    }

    printf("Talking to %i \n",port);


    // function for chat
    send_file(sockfd, argv[2]) ;

    printf("Done \n");




    // close the socket
    close(sockfd);
}
