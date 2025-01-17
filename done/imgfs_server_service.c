/*
 * @file imgfs_server_services.c
 * @brief ImgFS server part, bridge between HTTP server layer and ImgFS library
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>

#include "error.h"
#include "util.h" // atouint16
#include "imgfs.h"
#include "http_net.h"
#include "imgfs_server_service.h"

// Main in-memory structure for imgFS
static struct imgfs_file fs_file;
static uint16_t server_port;
pthread_mutex_t thread;

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
/**
 * Locks the mutex thread
 * @return ERR_RUNTIME if error and ERR_NONE if not
 */
int thread_lock()
{

    if (pthread_mutex_lock(&thread) != 0) {
        perror("pthread_mutex_lock failed");
        return ERR_RUNTIME;
    }
    return ERR_NONE;
}
/**
 * Unlocks the mutex thread
 * @return ERR_RUNTIME if error and ERR_NONE if not
 */
int thread_unlock()
{
    if (pthread_mutex_unlock(&thread) != 0) {
        perror("pthread_mutex_unlock failed");
        return ERR_RUNTIME;
    }
    return ERR_NONE;
}

int handle_list_call(int connection)
{

    char *json_op = NULL;

    // Locking the mutex before calling do_do_list
    if (thread_lock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);

    int list_ret = do_list(&fs_file, JSON, &json_op);

    // Unlocking the mutex after calling do_do_list
    if(thread_unlock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);


    if (list_ret != ERR_NONE) {
        if (json_op) free(json_op);
        return reply_error_msg(connection, list_ret);
    }

    if (json_op) {
        int ret = http_reply(connection, HTTP_OK, "Content-Type: application/json\r\n",
                             json_op, strlen(json_op));

        free(json_op);
        return ret;
    }

    return reply_error_msg(connection, ERR_RUNTIME);
}


int handle_read_call(int connection, const struct http_message* msg)
{

    M_REQUIRE_NON_NULL(msg);

    char res_str[15] = {0};
    char img_id[MAX_IMG_ID] = {0};

    // GetING res and imgID from the image's URI
    if (http_get_var(&msg->uri, "res", res_str, sizeof(res_str)) == 0 ||
        http_get_var(&msg->uri, "img_id", img_id, sizeof(img_id)) == 0) {
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    // Converting resolution string to int value
    int res = resolution_atoi(res_str);
    if (res == -1) {
        return reply_error_msg(connection, ERR_RESOLUTIONS);
    }

    char *image_buffer;
    uint32_t image_size;

    // Locking the mutex before calling do_read
    if (thread_lock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);

    int ret_read = do_read(img_id, res, &image_buffer, &image_size, &fs_file);

    // Unlocking the mutex after calling do_read
    if(thread_unlock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);

    if (ret_read != ERR_NONE) {
        return reply_error_msg(connection, ret_read);
    }

    char headers[MAX_HEADER_SIZE];
    snprintf(headers, sizeof(headers), "Content-Type: image/jpeg\r\n");

    //Sending HTTP response with the image
    int http_ret = http_reply(connection, "200 OK", headers, image_buffer, image_size);

    free(image_buffer);
    image_buffer = NULL;

    return http_ret;
}


int handle_delete_call(int connection, const struct http_message* msg)
{

    M_REQUIRE_NON_NULL(msg);

    char img_id[MAX_IMG_ID] = {0};

    // Extracting imgID from image's URI
    if (http_get_var(&msg->uri, "img_id", img_id, sizeof(img_id)) == 0) {
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    // Locking the mutex before calling do_delete
    if (thread_lock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);

    int ret_delete = do_delete(img_id, &fs_file);

    // Unlocking the mutex after calling do_delete
    if(thread_unlock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);

    if (ret_delete != ERR_NONE) {
        return reply_error_msg(connection, ret_delete);
    }

    return reply_302_msg(connection);
}

int handle_insert_call(int connection, const struct http_message* msg)
{

    M_REQUIRE_NON_NULL(msg);

    char name[256] = {0};

    // Extracting name from URI
    if (http_get_var(&msg->uri, "name", name, sizeof(name)) == 0) {
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    // Get image content from POST body
    const char *image_buffer = msg->body.val;
    size_t image_size = msg->body.len;

    // Lock the mutex before calling do_insert
    if (thread_lock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);

    int ret = do_insert(image_buffer, image_size, name, &fs_file);

    // Unlock the mutex after calling do_insert
    if(thread_unlock() != ERR_NONE) return reply_error_msg(connection, ERR_RUNTIME);

    if (ret != ERR_NONE) {
        return reply_error_msg(connection, ret);
    }

    // Send redirect response to client
    return reply_302_msg(connection);
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

    if (http_match_verb(&msg->uri, "/") || http_match_uri(msg, "/index.html")) {
        return http_serve_file(connection, BASE_FILE);
    }

    if (http_match_verb(&msg->method, "GET")) {
        if (http_match_uri(msg, URI_ROOT "/list")) {
            return handle_list_call(connection);
        }
        if (http_match_uri(msg, URI_ROOT "/read")) {
            return handle_read_call(connection, msg);
        }
        if (http_match_uri(msg, URI_ROOT "/delete")) {
            return handle_delete_call(connection, msg);
        }
    }

    if (http_match_verb(&msg->method, "POST") &&
        http_match_uri(msg, URI_ROOT "/insert")) {
        return handle_insert_call(connection, msg);
    }

    return reply_error_msg(connection, ERR_INVALID_COMMAND);
}


/********************************************************************//**
 * Startup function. Create imgFS file and load in-memory structure.
 * Pass the imgFS file name as argv[1] and optionally port number as argv[2]
 ********************************************************************** */
int server_startup (int argc, char **argv)
{

    //Argument validity check
    M_REQUIRE_NON_NULL(argv);
    if (argc < 2) return ERR_NOT_ENOUGH_ARGUMENTS;

    //Vips initialization
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(NULL);
    }

    int ret_open = do_open(argv[1], "rb+", &fs_file);

    if (ret_open < 0) return ret_open;

    print_header(&fs_file.header);

    // Using 2nd argument as the port number if present
    server_port = (argc > 2) ? atouint16(argv[2]) : DEFAULT_LISTENING_PORT;

    // Initialize the global mutex
    if (pthread_mutex_init(&thread, NULL) != 0) {
        perror("pthread_mutex_init failed");
        return ERR_RUNTIME;
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
    vips_shutdown();

    // Destroying the global mutex
    if (pthread_mutex_destroy(&thread) != 0) {
        perror("pthread_mutex_destroy failed");
    }
}