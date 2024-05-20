#include "http_prot.h"
#include <string.h>
#include <stdlib.h> // for malloc
#include "error.h"


/**
 * @brief Checks whether the `message` URI starts with the provided `target_uri`.
 *
 * Returns: 1 if it does, 0 if it does not.
 *
 */
int http_match_uri(const struct http_message *message, const char *target_uri) {
    M_REQUIRE_NON_NULL(message);
    M_REQUIRE_NON_NULL(target_uri);

    if (message->uri.len < strlen(target_uri)) {
        return 0;
    }

    for (size_t i = 0; i < strlen(target_uri); ++i) {
        if (message->uri.val[i] != target_uri[i]) {
            return 0;
        }
    }

    //case where message URI matches target URI
    return 1;
}


int http_match_verb(const struct http_string *method, const char *verb) {
    M_REQUIRE_NON_NULL(method);
    M_REQUIRE_NON_NULL(verb);

    size_t verb_length = strlen(verb);
    if (method->len != verb_length) {
        return 0;
    }

    for (size_t i = 0; i < verb_length; ++i) {
        if (method->val[i] != verb[i]) {
            return 0;
        }
    }

    return 1;
}

int http_get_var(const struct http_string *url, const char *name, char *out, size_t out_len) {

    M_REQUIRE_NON_NULL(url);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(out);
    if (out_len <= 0) return ERR_INVALID_ARGUMENT;

    //copy name into new string and append "=" (and null terminator)
    size_t name_length = strlen(name);
    char *param_eq = (char *) malloc(name_length + 2); // 1 for "=" and 1 for the nul terminator ('\0')!!
    if (!param_eq) {
        return ERR_OUT_OF_MEMORY;
    }
    strcpy(param_eq, name);
    strcat(param_eq, "=\0");

    //find parameter position in URL
    char *start = strstr(url->val, param_eq);
    if (!start) {
        free(param_eq);
        return 0; // parameter not found in URL -> return 0 (handout)
    }

    start += name_length + 1;
    char *end = strchr(start, '&');
    if (!end) {
        end = (char *) url->val + url->len;  //if no & is found, the value goes until the end of the URL (handout)
    }

    size_t value_length = end - start;
    if (value_length >= out_len) {
        free(param_eq);
        return ERR_RUNTIME;
    }

    strncpy(out, start, value_length);
    out[value_length] = '\0';

    free(param_eq);
    return value_length;
}

//==================================================================================================================
//============================================== PARSE HTTP MESSAGES ===============================================
//==================================================================================================================

/**
 * @brief: Extract the first substring (prefix) of a the string before some delimiter
 */

const char *get_next_token(const char *message, const char *delimiter, struct http_string *output) {
    const char *delim_pos = strstr(message, delimiter);
    if (delim_pos) {
        output->val = message;
        output->len = delim_pos - message;

        return delim_pos + strlen(delimiter);
    }
    output->val = message;
    output->len = strlen(message);

    return message + strlen(message);
}

/**
 * @brief: Fill all headers key-value pairs of output
 */
const char *http_parse_headers(const char *header_start, struct http_message *output) {
    const char *current = header_start;
    struct http_string key, value;
    size_t idx = 0;

    while (*current && idx < MAX_HEADERS) {
        current = get_next_token(current, HTTP_HDR_KV_DELIM, &key);
        // extract the value using  end-of-line as the delimiter.
        current = get_next_token(current, HTTP_LINE_DELIM, &value);

        // break if key or value are empty (meaning a parsing issue or end of headers)
        if (key.len == 0 || value.len == 0) {
            break;
        }

        // store extracted key + value in the output
        output->headers[idx].key = key;
        output->headers[idx].value = value;
        idx++;
    }
    // set nb of parsed headers.
    output->num_headers = idx;

    // return position right after end of the last header line.
    return current; //+ strlen(HTTP_LINE_DELIM);
}

/**
 * @see {http_prot.h#http_parse_message}
 */
int http_parse_message(const char *stream, size_t bytes_received, struct http_message *out, int *content_len) {
    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(out);
    M_REQUIRE_NON_NULL(content_len);

    // check that header has been completely received
    const char *header_end = strstr(stream, HTTP_HDR_END_DELIM);
    if (!header_end) {
        return 0;  // headers incomplete
    }

    //CHANGE_SARA : added parsing the first line  (see handout )
    const char *current = stream;
    current = get_next_token(current, " ", &out->method);
    current = get_next_token(current, " ", &out->uri);
    struct http_string http_version;
    current = get_next_token(current, HTTP_LINE_DELIM, &http_version);


    // parse headers
    //const char *body_start = http_parse_headers(stream, out);
    // Parse headers
    current = http_parse_headers(current, out);

    if (out->num_headers == 0) {
        return 0;  // no headers parsed
    }


    // extract content length from headers
    *content_len = 0;
    for (size_t i = 0; i < out->num_headers; i++) {
        if (strncmp(out->headers[i].key.val, "Content-Length", out->headers[i].key.len) == 0) {
            *content_len = atoi(out->headers[i].value.val);
            break;
        }
    }

    //CHANGE_SARA : condition update
    size_t header_length = header_end + strlen(HTTP_HDR_END_DELIM) - stream;
    size_t total_length = header_length + *content_len;
    if (*content_len > 0 && bytes_received < total_length) {
        return 0; // Message is incomplete (body not fully received)
    }

    // handle body (if any)
    if (*content_len > 0) {
        if (bytes_received < total_length){
            out->body.val = NULL;
            out->body.len = 0;  //incomplete body
            return 0;
        }else{
            out->body.val = header_end + strlen(HTTP_HDR_END_DELIM);
            out->body.len = *content_len;
        }


    }


    return 1;  // msg fully received and parsed w/o a body
}

//==================================================================================================================
//==================================================================================================================
//==================================================================================================================



