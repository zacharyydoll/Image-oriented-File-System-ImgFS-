#include "socket_layer.h"
#include "error.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

/**
 * This is a help function that logs an error message and closes a given socket,
 * then returns a constant error code.
 *
 * @param err_mess  The error message to be logged.
 * @param socket_tcp  The socket to be closed.
 *
 * @return ERR_IO  A constant int that represents an I/O error.
 */
int close_after_error(char* err_mess,int socket_tcp)
{
    perror(err_mess);
    close(socket_tcp);
    return ERR_IO;
}


int tcp_server_init(uint16_t port)
{
    int socket_tcp ;
    struct sockaddr_in server_addr;

    // Clear out server_addr structure
    memset(&server_addr, 0, sizeof(server_addr));

    socket_tcp = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(socket_tcp<0) {
        close_after_error("Error creating socket",socket_tcp);
    }

    bzero(&server_addr, sizeof(server_addr));
    // Use Internet address family
    server_addr.sin_family = AF_INET;
    // Let socket choose the IP address
    server_addr.sin_addr.s_addr = INADDR_ANY;
    // Convert and set port number
    server_addr.sin_port = htons(port);


    //Binding the socket
    if (bind(socket_tcp, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        close_after_error("Error binding socket",socket_tcp);
    }

    //Listening
    if (listen(socket_tcp, SOMAXCONN) < 0) { //not sure : CHECK HOW MANY connections
        close_after_error("Error listening on socket",socket_tcp);
    }

    return socket_tcp;
}

int tcp_accept(int passive_socket)
{
    return accept(passive_socket,NULL,NULL);
}

ssize_t tcp_read(int active_socket, char* buf, size_t buflen)
{
    M_REQUIRE_NON_NULL(buf);
    return recv(active_socket,buf,buflen,0);
}

ssize_t tcp_send(int active_socket, const char* response, size_t response_len)
{
    M_REQUIRE_NON_NULL(response);
    return send(active_socket,response,response_len,0);
}