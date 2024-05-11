#include "http_prot.h"
#include <string.h>
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

    if (method->len != strlen(verb) - 1) {
        return 0;
    }

    for (size_t i = 0; i < strlen(verb) - 1; ++i) {
        if (method->val[i] != verb[i]) {
            return 0;
        }
    }

    return 1;
}
