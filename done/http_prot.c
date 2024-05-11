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


int http_match_verb(const struct http_string* method, const char* verb) {
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

int http_get_var(const struct http_string* url, const char* name, char* out, size_t out_len) {

    M_REQUIRE_NON_NULL(url);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(out);
    if(out_len <= 0) return ERR_INVALID_ARGUMENT;

    //copy name into new string and append "=" (and null terminator)
    size_t name_length = strlen(name);
    char* param_eq = (char*)malloc(name_length + 2); // 1 for "=" and 1 for the nul terminator ('\0')!!
    if (!param_eq) {
        return ERR_OUT_OF_MEMORY;
    }
    strcpy(param_eq, name);
    strcat(param_eq, "=\0");

    //find parameter position in URL
    char* start = strstr(url->val, param_eq);
    if (!start) {
        free(param_eq);
        return 0; // parameter not found in URL -> return 0 (handout)
    }

    start += name_length + 1;
    char* end = strchr(start, '&');
    if (!end) {
        end = (char*) url->val + url->len;  //if no '&' is found, the value goes until the end of the URL (handout)
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


