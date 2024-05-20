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

    //buffer for the http header - used allocation so that I can assign new value to it
    char *rcvbuf = malloc(MAX_HEADER_SIZE);
    if (rcvbuf == NULL) return &our_ERR_OUT_OF_MEMORY;

    int read_bytes = 0;
    char *header_end = NULL;
    int content_len = 0; // ZAC: explicitly initialized content length to 0.
    int extended = 0;

    struct http_message message;

    do {
        ssize_t num_bytes_read = tcp_read(client_fd,
                                          rcvbuf + read_bytes,
                                          MAX_HEADER_SIZE - read_bytes - 1);
        if (num_bytes_read < 0) {
            free(rcvbuf);
            return &our_ERR_IO;
        }
        read_bytes += (int)num_bytes_read;
        rcvbuf[read_bytes] = '\0'; // null terminate string for safety

        // search for the header delimiter
        header_end = strstr(rcvbuf, HTTP_HDR_END_DELIM);  // "\r\n\r\n"

        //=============================================WEEK 12==========================================================

        int ret_parsed_mess = http_parse_message(rcvbuf,read_bytes,&message, &content_len);
        if (ret_parsed_mess < 0) {
            free(rcvbuf); // parse_message returns negative if an error occurred (http_prot.h)
            return &our_ERR_IO;
        } else if (ret_parsed_mess == 0) { //partial treatment (see http_prot.h)
            if (!extended && content_len > 0 && read_bytes < MAX_HEADER_SIZE + content_len) {
                char *new_buf = realloc(rcvbuf, MAX_HEADER_SIZE + content_len);
                if (!new_buf) {
                    free(rcvbuf);
                    return &our_ERR_OUT_OF_MEMORY;
                }
                rcvbuf = new_buf;
                extended = 1;
            }
        } else { // case where the message was fully received and parsed
            int callback_result = cb(&message, client_fd);
            if (callback_result < 0) {
                free(rcvbuf);
                return &our_ERR_IO;
            } else {
                read_bytes = 0;
                content_len = 0;
                extended = 0;
                memset(rcvbuf, 0, MAX_HEADER_SIZE);
            }
        }
    } while (!header_end && read_bytes < MAX_HEADER_SIZE);  // do this until delimiter is found, or buffer is full

    free(rcvbuf); // avoid memory leaks :)
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
int http_serve_file(int connection, const char* filename)
{
    M_REQUIRE_NON_NULL(filename);

    // open file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to open file \"%s\"\n", filename);
        return http_reply(connection, "404 Not Found", "", "", 0);
    }

    // get its size
    fseek(file, 0, SEEK_END);
    const long pos = ftell(file);
    if (pos < 0) {
        fprintf(stderr, "http_serve_file(): Failed to tell file size of \"%s\"\n",
                filename);
        fclose(file);
        return ERR_IO;
    }
    rewind(file);
    const size_t file_size = (size_t) pos;

    // read file content
    char* const buffer = calloc(file_size + 1, 1);
    if (buffer == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to allocate memory to serve \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    const size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "http_serve_file(): Failed to read \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    // send the file
    const int  ret = http_reply(connection, HTTP_OK,
                                "Content-Type: text/html; charset=utf-8" HTTP_LINE_DELIM,
                                buffer, file_size);

    // garbage collecting
    fclose(file);
    free(buffer);
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
    size_t est_header_len = strlen(HTTP_PROTOCOL_ID) + 1 + strlen(status) + strlen(HTTP_LINE_DELIM) +
                            strlen(headers) + strlen("Content-Length: ") + 20 + strlen(HTTP_HDR_END_DELIM);

    size_t max_total_size = est_header_len + body_len;

    char *buffer = malloc(max_total_size + 1);  // +1 for null terminator
    if (!buffer) return ERR_OUT_OF_MEMORY;

    // header with format from handout (see https://www.geeksforgeeks.org/snprintf-c-library/)
    int header_len = snprintf(buffer, max_total_size + 1, "%s %s\r\n%sContent-Length: %zu\r\n\r\n",
                              HTTP_PROTOCOL_ID, status, headers, body_len);
    if (header_len < 0 || (size_t)header_len >= max_total_size + 1) {
        free(buffer);
        return ERR_RUNTIME;
    }

    // add body to the end of the buffer
    if (body && body_len > 0) {
        memcpy(buffer + header_len, body, body_len);
    }

    ssize_t total_len = header_len + body_len;
    ssize_t sent_len = tcp_send(connection, buffer, total_len);

    free(buffer);
    return (sent_len == total_len) ? ERR_NONE : ERR_IO;
}
