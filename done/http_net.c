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
#include <pthread.h>

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
 * @brief Helper function to handle cleanup and return error code.
 *
 * @param client_fd The client file descriptor.
 * @param arg The argument to be freed.
 * @param rcvbuf The pointer to the buffer to be freed.
 * @param error_code The error code to return.
 * @return The error code passed as an argument.
 */
static void *safe_free(int client_fd, void* arg, char* rcvbuf, void* error_code) {
    if (arg == NULL || rcvbuf == NULL || error_code == NULL) return &our_ERR_INVALID_ARGUMENT;
    free(rcvbuf);
    //close(client_fd);
    free(arg);
    return error_code;
}


/*******************************************************************
 * @brief Handle the client connection and process the HTTP message.
 *
 * @param arg The argument passed to the thread (client file descriptor).
 * @return The error code on failure, or success code on completion.
 */
static void *handle_connection(void *arg) {
    if (arg == NULL) return &our_ERR_INVALID_ARGUMENT;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    int client_fd = *(int *)arg;



    ssize_t read_bytes = 0;
    char *header_end = NULL;
    int content_len = 0;
    int extended = 0;

    struct http_message message;
    int parse_result = 0; // ZAC 27.05: add parse_result to track parsing
    ssize_t num_bytes_read = -7;

    size_t max_buff_sz = MAX_HEADER_SIZE; // ZAC 27.05: using max_buffer_size for dynamic resizing
    char *rcvbuf = calloc(1,max_buff_sz); // ZAC 27.05: initialize buffer with max_buffer_size
    if (rcvbuf == NULL) {
        free(arg); // ZAC 27.05: free the argument before returning
        return &our_ERR_OUT_OF_MEMORY;
    }

    do {

        do {

        num_bytes_read = tcp_read(client_fd, rcvbuf + read_bytes,
                                  max_buff_sz - read_bytes - 1);

        if (num_bytes_read < 0) {
            free(rcvbuf);
            return &our_ERR_IO;
        }
        read_bytes += num_bytes_read;
        rcvbuf[read_bytes] = '\0'; // null terminate string for safety
        header_end = strstr(rcvbuf, HTTP_HDR_END_DELIM); // "\r\n\r\n"


            parse_result = http_parse_message(rcvbuf, read_bytes, &message, &content_len);
            if (parse_result < 0) {
                free(rcvbuf);
                return &our_ERR_IO;
            }

            if (parse_result == 0 && content_len > 0 && read_bytes < content_len) {
                max_buff_sz = MAX_HEADER_SIZE + content_len; // ZAC 27.05: update max_buffer_size
                char *new_buf = realloc(rcvbuf, max_buff_sz); // ZAC 27.05: use max_buffer_size for realloc
                if (!new_buf) {
                    free(rcvbuf);
                    return &our_ERR_IO;
                }

                rcvbuf = new_buf;
                extended = 1;
            }
        } while (parse_result == 0 && read_bytes < max_buff_sz); // NEW VERSION: use parse_result and max_buffer_size

        if (parse_result > 0) { // ZAC 27.05: process the fully parsed message
            if (cb) {
                int callback_result = cb(&message, client_fd); // NEW VERSION: handle callback result
                if (callback_result < 0){
                    free(rcvbuf);
                    return &our_ERR_IO;
                }


            }

        }
        read_bytes = 0;
            content_len = 0;
            extended = 0;
            memset(rcvbuf,0,max_buff_sz);

    } while( num_bytes_read != 0);

    //free(rcvbuf);
    close(client_fd);
    free(arg);
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
        if (close(passive_socket) == -1) {
            perror("close() in http_close()");
        }
        else {
            passive_socket = -1;
        }
    }
}

/*******************************************************************
 * Receive content
 */
int http_receive(void) {
    //connects to socket with tcp_accept (returns ERR_IO if fails),
    // and handles the connection through handle_connection().
    pthread_attr_t thread ;
    int ret;
    int* active_socket = (int*) calloc(1,sizeof(int));

    if (!active_socket) {
        // Memory allocation failed, return error
        free(active_socket);
        return ERR_OUT_OF_MEMORY;
    }
    *active_socket = tcp_accept(passive_socket);
    if (*active_socket < 0) {
        free(active_socket);
        return ERR_IO;
    }
    // Initialize the thread attributes
    ret = pthread_attr_init(&thread);
    if (ret) {
        free(active_socket);
        return ERR_IO;
    }
    ret = pthread_attr_setdetachstate(&thread, PTHREAD_CREATE_DETACHED);
    if(ret){
        pthread_attr_destroy(&thread); // Error handling, destroy the initialized attributes before return
        free(active_socket);
        return ERR_IO;
    }
    ret = pthread_create(&thread, NULL, handle_connection, (void *) active_socket);
    if (ret) {
        pthread_attr_destroy(&thread);
        free(active_socket);
        return ERR_IO;
    }

    // Once done with the connection, free the active socket
    pthread_attr_destroy(&thread);
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
    const int EXTRA_LENGTH = 20;

    if (body == NULL && body_len > 0) {
        return ERR_INVALID_ARGUMENT; // body can be null for responses with empty body, but then length should be 0
    }

    // Compute required buffer size
    size_t est_header_len = strlen(HTTP_PROTOCOL_ID) + strlen(status) + strlen(HTTP_LINE_DELIM) +
                            strlen(headers) + strlen("Content-Length: ") + EXTRA_LENGTH + strlen(HTTP_HDR_END_DELIM);

    size_t max_total_size = est_header_len + body_len;

    char *buffer = malloc(max_total_size + 1);  // +1 for null terminator
    if (!buffer) return ERR_OUT_OF_MEMORY;

    // ensure status string starts with space after HTTP version
    int header_len = snprintf(buffer, max_total_size + 1, "%s%s%s%sContent-Length: %zu%s",
                              HTTP_PROTOCOL_ID, status, HTTP_LINE_DELIM, headers, body_len, HTTP_HDR_END_DELIM);

    if (header_len < 0 || (size_t)header_len >= max_total_size + 1) {
        free(buffer);
        return ERR_RUNTIME;
    }

    // Add body to the end of the buffer
    if (body && body_len > 0) {
        memcpy(buffer + header_len, body, body_len);
    }

    ssize_t total_len = header_len + body_len;
    ssize_t sent_len = tcp_send(connection, buffer, total_len);

    free(buffer);
    return (sent_len == total_len) ? ERR_NONE : ERR_IO;
}
