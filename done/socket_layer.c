#include "socket_layer.h"
#include "error.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>


int tcp_server_init(uint16_t port){
    int socket_tcp ;
    struct sockaddr_in server_addr;

    // Clear out server_addr structure
    memset(&server_addr, 0, sizeof(server_addr));

    socket_tcp = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(socket_tcp<0){
        perror("Error creating socket");
        close(socket_tcp);
        return ERR_IO;
    }
    // Use Internet address family
    server_addr.sin_family = AF_INET;
    // Let socket choose the IP address
    server_addr.sin_addr.s_addr = INADDR_ANY;
    // Convert and set port number
    server_addr.sin_port = htons(port);


    //Binding the socket
    if (bind(socket_tcp, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(socket_tcp);
        return ERR_IO;
    }

    //Listening
    if (listen(socket_tcp, SOMAXCONN) < 0) { //not sure : CHECK HOW MANY connections
        perror("Error listening on socket");
        close(socket_tcp);
        return ERR_IO;
    }


    printf("Server started on port %d\n", port);

    return socket_tcp;
}

int tcp_accept(int passive_socket){
    return accept(passive_socket,NULL,NULL);
}

ssize_t tcp_read(int active_socket, char* buf, size_t buflen){
    M_REQUIRE_NON_NULL(buf);
    return read(active_socket,buf,buflen);
}

ssize_t tcp_send(int active_socket, const char* response, size_t response_len){
    M_REQUIRE_NON_NULL(response);
    return send(active_socket,response,response_len,0);
}