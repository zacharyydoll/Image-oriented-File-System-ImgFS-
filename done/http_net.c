/*
 * @file http_net.c
 * @brief HTTP server layer for CS-202 project
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "http_prot.h"
#include "http_net.h"
#include "socket_layer.h"
#include "error.h"
#include "imgfs.h"

static int passive_socket = -1;
static EventCallback cb;

#define MK_OUR_ERR(X) \
static int our_ ## X = X

MK_OUR_ERR(ERR_NONE);
MK_OUR_ERR(ERR_INVALID_ARGUMENT);
MK_OUR_ERR(ERR_OUT_OF_MEMORY);
MK_OUR_ERR(ERR_IO);

/*******************************************************************
 * @brief Helper function to safely free pointers.
 *
 * @param ptr The pointer to free
 */
void safe_free(void* ptr)
{
    if (ptr != NULL ) {
        free(ptr);
        ptr = NULL;
    }
}

/*******************************************************************
 * @brief Handle the client connection and process the HTTP message.
 *
 * @param arg The argument passed to the thread (client file descriptor).
 * @return The error code on failure, or success code on completion.
 */
static void *handle_connection(void *arg){

    //Argument validity check
    if (arg == NULL) return &our_ERR_INVALID_ARGUMENT;

    //Handling multi-threading
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    int client_fd = *(int *)arg;

    ssize_t read_bytes = 0;
    int content_len = 0;

    struct http_message message;
    int parse_result ;
    ssize_t num_bytes_read ;

    // using max_buffer_size for dynamic resizing
    size_t max_buff_sz = MAX_HEADER_SIZE;

    char *rcvbuf = calloc(1,max_buff_sz);

    if (rcvbuf == NULL) {
        safe_free(arg);
        return &our_ERR_OUT_OF_MEMORY;
    }

    //While loop on every connection set by the client
    do {

        do {

            num_bytes_read = tcp_read(client_fd, rcvbuf + read_bytes,
                                      max_buff_sz - read_bytes - 1);

            if (num_bytes_read < 0) {
                safe_free(rcvbuf);
                return &our_ERR_IO;
            }
            read_bytes += num_bytes_read;
            rcvbuf[read_bytes] = '\0'; // null terminate string for safety

            //Parsing the message until parsed completely
            parse_result = http_parse_message(rcvbuf, read_bytes, &message, &content_len);

            //Error in parsing
            if (parse_result < 0) {
                safe_free(rcvbuf);
                return &our_ERR_IO;
            }

            //Message not complete
            if (parse_result == 0 && content_len > 0 && read_bytes < content_len) {
                max_buff_sz = MAX_HEADER_SIZE + content_len;
                char *new_buf = realloc(rcvbuf, max_buff_sz);
                if (!new_buf) {
                    safe_free(rcvbuf);
                    return &our_ERR_IO;
                }
                rcvbuf = new_buf;
            }

        } while (parse_result == 0 && read_bytes < max_buff_sz);

        //Completely parsed message
        if (parse_result > 0) {
            if (cb) {
                int callback_result = cb(&message, client_fd);
                if (callback_result < 0) {
                    safe_free(rcvbuf);
                    return &our_ERR_IO;
                }
            }

        }
        //resetting buffer for another round of tcp_read
        read_bytes = 0;
        content_len = 0;
        memset(rcvbuf,0,max_buff_sz);

    } while( num_bytes_read != 0);

    //Closing socket after use
    close(client_fd);
    safe_free(arg);

    return &our_ERR_NONE;
}

/*******************************************************************
 * Init connection
 */
int http_init(uint16_t port, EventCallback callback)
{
    passive_socket = tcp_server_init(port);
    cb = callback;
    return passive_socket;
}

/*******************************************************************
 * Close connection
 */
void http_close(void)
{
    if (passive_socket > 0) {
        if (close(passive_socket) == -1) {
            perror("close() in http_close()");
        } else {
            passive_socket = -1;
        }
    }
}

/*******************************************************************
 * Receive content
 */
int http_receive(void)
{
    pthread_attr_t thread ;
    int ret;
    int* active_socket = (int*) calloc(1,sizeof(int));

    if (!active_socket) {
        // Memory allocation failed, return error
        safe_free(active_socket);
        return ERR_OUT_OF_MEMORY;
    }
    //Connecting to socket with tcp_accept
    *active_socket = tcp_accept(passive_socket);

    if (*active_socket < 0) {
        safe_free(active_socket);
        return ERR_IO;
    }

    //Initializing the thread attributes
    ret = pthread_attr_init(&thread);
    if (ret) {
        safe_free(active_socket);
        return ERR_IO;
    }

    ret = pthread_attr_setdetachstate(&thread, PTHREAD_CREATE_DETACHED);

    if(ret) {
        pthread_attr_destroy(&thread); // Error handling, destroy the initialized attributes before return
        free(active_socket);
        return ERR_IO;
    }

    //Handling the connection through handle_connection().
    ret = pthread_create((pthread_t *) &thread, NULL, handle_connection,
                         (void *) active_socket);

    if (ret) {
        pthread_attr_destroy(&thread);
        safe_free(active_socket);
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

    //Opening the file
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to open file \"%s\"\n", filename);
        return http_reply(connection, "404 Not Found", "", "", 0);
    }

    //Getting its size
    fseek(file, 0, SEEK_END);
    const long pos = ftell(file);

    if (pos < 0) {
        fprintf(stderr, "http_serve_file(): Failed to tell file size of \"%s\"\n",filename);
        fclose(file);
        return ERR_IO;
    }
    rewind(file);
    const size_t file_size = (size_t) pos;

    //Reading file content
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

    //Sending the file
    const int  ret = http_reply(connection, HTTP_OK,
                                "Content-Type: text/html; charset=utf-8" HTTP_LINE_DELIM,
                                buffer, file_size);

    fclose(file);
    safe_free(buffer);
    return ret;
}

/*******************************************************************
 * Create and send HTTP reply
 */
int http_reply(int connection, const char* status, const char* headers, const char *body, size_t body_len)
{
    //Argument validity check
    M_REQUIRE_NON_NULL(status);
    M_REQUIRE_NON_NULL(headers);
    const int EXTRA_LENGTH = 20;

    if (body == NULL && body_len > 0) {
        return ERR_INVALID_ARGUMENT; // body can be null for responses with empty body, but then length should be 0
    }

    // Computing required buffer size
    size_t est_header_len = strlen(HTTP_PROTOCOL_ID) + strlen(status) + strlen(HTTP_LINE_DELIM) +
                            strlen(headers) + strlen("Content-Length: ") + EXTRA_LENGTH + strlen(HTTP_HDR_END_DELIM);

    size_t max_total_size = est_header_len + body_len;

    char *buffer = malloc(max_total_size + 1);  // +1 for null terminator
    if (!buffer) return ERR_OUT_OF_MEMORY;

    // Ensuring  status string starts with space after HTTP version
    int header_len = snprintf(buffer, max_total_size + 1, "%s%s%s%sContent-Length: %zu%s",
                              HTTP_PROTOCOL_ID, status, HTTP_LINE_DELIM, headers, body_len, HTTP_HDR_END_DELIM);

    if (header_len < 0 || (size_t)header_len >= max_total_size + 1) {
        safe_free(buffer);
        return ERR_RUNTIME;
    }

    // Add body to the end of the buffer
    if (body && body_len > 0) {
        memcpy(buffer + header_len, body, body_len);
    }

    size_t total_len = header_len + body_len;
    size_t sent_len = tcp_send(connection, buffer, total_len);

    safe_free(buffer);
    return (sent_len == total_len) ? ERR_NONE : ERR_IO;
}
