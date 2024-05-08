/* 
 * @file http_net.c
 * @brief HTTP server layer for CS-202 project
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include "http_prot.h"
#include "http_net.h"
#include "socket_layer.h"
#include "error.h"

static int passive_socket = -1;
static EventCallback cb;

#define MK_OUR_ERR(X) \
static int our_ ## X = X

MK_OUR_ERR(ERR_NONE);
MK_OUR_ERR(ERR_INVALID_ARGUMENT);
MK_OUR_ERR(ERR_OUT_OF_MEMORY);
MK_OUR_ERR(ERR_IO);

/*******************************************************************
 * Handle connection
 */
static void *handle_connection(void *arg) {

    if (arg == NULL) return &our_ERR_INVALID_ARGUMENT;
    int client_fd = *(int*)arg;

    char buffer[MAX_HEADER_SIZE];
    int read_bytes = 0;
    char *header_end = NULL;

    do {
        ssize_t num_bytes_read = tcp_read(client_fd,
                                          buffer + read_bytes,
                                          sizeof(buffer) - read_bytes - 1);
        if (num_bytes_read < 0) {
            return &our_ERR_IO;  //error reading
        }
        read_bytes += (int)num_bytes_read; // redundant, but was to avoid implicit typecasting
        buffer[read_bytes] = '\0';  // null terminate string for safety

        // Search for the header delimiter
        header_end = strstr(buffer, HTTP_HDR_END_DELIM);  // "\r\n\r\n"
    } while (!header_end && read_bytes < MAX_HEADER_SIZE);  // do this until delimiter is found, or buffer is full

    if (!header_end) return &our_ERR_IO; //case where header delimiter is not found

    const char *status = strstr(buffer, "test: ok") ? HTTP_OK : HTTP_BAD_REQUEST;
    http_reply(client_fd, status, "", NULL, 0);  // other parameters can be empty (see handout)

    return &our_ERR_NONE;
}


/*******************************************************************
 * Init connection
 */
int http_init(uint16_t port, EventCallback callback) {
    passive_socket = tcp_server_init(port);
    cb = callback;
    return passive_socket;
}

/*******************************************************************
 * Close connection
 */
void http_close(void) {
    if (passive_socket > 0) {
        if (close(passive_socket) == -1)
            perror("close() in http_close()");
        else
            passive_socket = -1;
    }
}

/*******************************************************************
 * Receive content
 */
int http_receive(void) {
    //connects to socket with tcp_accept (returns ERR_IO if fails),
    // and handles the connection through handle_connection().
    int client_fd = tcp_accept(passive_socket);
    if (client_fd < 0) {
        return ERR_IO;
    }

    handle_connection((void*)&client_fd);
    return ERR_NONE;
}

/*******************************************************************
 * Serve a file content over HTTP
 */
int http_serve_file(int connection, const char* filename) {
    int ret = ERR_NONE;
    return ret;
}

/*******************************************************************
 * Create and send HTTP reply
 */
int http_reply(int connection, const char* status, const char* headers, const char *body, size_t body_len) {
    M_REQUIRE_NON_NULL(status);
    M_REQUIRE_NON_NULL(headers);

    if(body == NULL && body_len > 0) {
        return ERR_INVALID_ARGUMENT; // body can be null for responses with empty body, but then length should be 0
    }

    // compute required buffer size
    size_t est_header_len = strlen(HTTP_PROTOCOL_ID) + strlen(status) + strlen(HTTP_LINE_DELIM) +
                        strlen(headers) + 50; //counted about 50 for "Content-Length: " and length + delimiters
    size_t max_total_size = est_header_len + body_len + strlen(HTTP_HDR_END_DELIM);

    char *buffer = malloc(max_total_size + 1);  // +1 for null terminator
    if (!buffer) return ERR_OUT_OF_MEMORY;

    // header with format from handout (see https://www.geeksforgeeks.org/snprintf-c-library/)
    snprintf(buffer, max_total_size + 1, "%s %s\r\n%sContent-Length: %zu\r\n\r\n",
             HTTP_PROTOCOL_ID, status, headers, body_len);

    // add body to the end of the buffer
    if (body && body_len > 0) {
        memcpy(buffer + strlen(buffer), body, body_len);
    }

    // Send everything to the socket
    tcp_send(connection, buffer, strlen(buffer));

    free(buffer);
    return ERR_NONE;
}
