/* 
 * @file imgfs_server_services.c
 * @brief ImgFS server part, bridge between HTTP server layer and ImgFS library
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // uint16_t

#include "error.h"
#include "util.h" // atouint16
#include "imgfs.h"
#include "http_net.h"
#include "imgfs_server_service.h"

// Main in-memory structure for imgFS
static struct imgfs_file fs_file;
static uint16_t server_port;

#define URI_ROOT "/imgfs"

/**********************************************************************
 * Sends error message.
 ********************************************************************** */
static int reply_error_msg(int connection, int error)
{
#define ERR_MSG_SIZE 256
    char err_msg[ERR_MSG_SIZE]; // enough for any reasonable err_msg
    if (snprintf(err_msg, ERR_MSG_SIZE, "Error: %s\n", ERR_MSG(error)) < 0) {
        fprintf(stderr, "reply_error_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "500 Internal Server Error", "",
                      err_msg, strlen(err_msg));
}

/**********************************************************************
 * Sends 302 OK message.
 ********************************************************************** */
static int reply_302_msg(int connection)
{
    char location[ERR_MSG_SIZE];
    if (snprintf(location, ERR_MSG_SIZE, "Location: http://localhost:%d/" BASE_FILE HTTP_LINE_DELIM,
                 server_port) < 0) {
        fprintf(stderr, "reply_302_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "302 Found", location, "", 0);
}

/**********************************************************************
 * Simple handling of http message. TO BE UPDATED WEEK 13
 ********************************************************************** */
int handle_http_message(struct http_message* msg, int connection)
{
    M_REQUIRE_NON_NULL(msg);
    debug_printf("handle_http_message() on connection %d. URI: %.*s\n",
                 connection,
                 (int) msg->uri.len, msg->uri.val);
    if (http_match_uri(msg, URI_ROOT "/list")      ||
        (http_match_uri(msg, URI_ROOT "/insert")
         && http_match_verb(&msg->method, "POST")) ||
        http_match_uri(msg, URI_ROOT "/read")      ||
        http_match_uri(msg, URI_ROOT "/delete"))
        return reply_302_msg(connection);
    else
        return reply_error_msg(connection, ERR_INVALID_COMMAND);
}


/********************************************************************//**
 * Startup function. Create imgFS file and load in-memory structure.
 * Pass the imgFS file name as argv[1] and optionnaly port number as argv[2]
 ********************************************************************** */
int server_startup (int argc, char **argv) {
    if (argc < 2) return ERR_NOT_ENOUGH_ARGUMENTS;
    M_REQUIRE_NON_NULL(argv);

    int ret_open = do_open(argv[1], "r+", &fs_file);
    if (ret_open < 0) {
        return ret_open;
    }

    print_header(&fs_file.header);

    //2nd argument is the port number and is optional. If present, use it.
    if (argc > 2) {
        server_port = atouint16(argv[2]);
    } else {
        server_port = DEFAULT_LISTENING_PORT;
    }

    http_init(server_port, handle_http_message);
    printf("ImgFS server started on http://localhost:%u\n", server_port);
    return ERR_NONE;
}

/********************************************************************//**
 * Shutdown function. Free the structures and close the file.
 ********************************************************************** */
void server_shutdown (void)
{
    fprintf(stderr, "Shutting down...\n");
    http_close();
    do_close(&fs_file);
}



