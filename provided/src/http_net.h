/**
 * @file http_net.h
 * @brief Minimal HTTP network services.
 *
 * @author Konstantinos Prasopoulos
 */

#pragma once

#include <stdint.h>
#include "http_prot.h" // for structs

#define MAX_REQUEST_SIZE 8388608 // 2^23 -> to handle images up to 8MB
#define MAX_HEADER_SIZE    16384 // 2^14 -> to handle http headers

// Defined as specified in handout of week 11
typedef int (*EventCallback)(struct http_message*, int);

int http_init(uint16_t port, EventCallback cb);

int http_receive(void);

int http_serve_file(int connection, const char* filename);

int http_reply(int connection, const char* status, const char* headers, const char* body, size_t body_len);

void http_close(void);
