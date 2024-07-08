#define IN_CS202_UNIT_TEST
#include "http_prot.h"
#include <check.h>
#include "test.h"

const char *get_next_token(const char *message, const char *delimiter, struct http_string *output);
const char *http_parse_headers(const char *header_start, struct http_message *output);


#define ck_assert_http_str_eq(str, expect) \
    ck_assert_msg(strncmp(str.val, expect, str.len) == 0 && strlen(expect) == str.len, \
                  "Expected '%s', but got '%.*s'", expect, (int)str.len, str.val)



START_TEST(test_http_match_uri_prefix)
{
    struct http_message msg1 = {.uri = {"https://localhost:8000/imgfs/read?res=orig&img_id=mure.jpg", 55}};
    struct http_message msg2 = {.uri = {"/universal/resource/identifier", 27}};


    // ===========================================================

    ck_assert(http_match_uri(&msg1, "https://") == 1);
    ck_assert(http_match_uri(&msg1, "https://localhost:8000/") == 1);
    ck_assert(http_match_uri(&msg1, "https://localhost:8000/imgfs") == 1);

    ck_assert(http_match_uri(&msg1, "http://") == 0);

    //=============================================================

    ck_assert(http_match_uri(&msg2, "/universal/resource") == 1);
    ck_assert(http_match_uri(&msg2, "/universal") == 1);

    ck_assert(http_match_uri(&msg2, "/universe") == 0);

    //=============================================================

}
END_TEST


START_TEST(test_http_match_verb_exact)
{
    struct http_string method1 = {.val = "POST / HTTP/1.1", .len = 4};
    struct http_string method2 = {.val = "GET / HTTP/1.1" , .len = 3};

    // Test for exact matches
    ck_assert(http_match_verb(&method1, "POST") == 1);

    // Test for non-matches
    //ck_assert(http_match_verb(&method1, "POS") == 0);
    //ck_assert(http_match_verb(&method1, "POST ") == 0);

    ck_assert(http_match_verb(&method2, "GET /") == 0);
    ck_assert(http_match_verb(&method2, "G") == 0);

}
END_TEST



START_TEST(test_http_get_var_found) {
    struct http_string url1 = {.val = "http://localhost:8000/imgfs/read?res=orig&img_id=mure.jpg",
            .len = strlen("http://localhost:8000/imgfs/read?res=orig&img_id=mure.jpg")};

    struct http_string url2 = {.val = "http://localhost:8000/imgfs/read?max_file=100&img_id=mure.jpg",
            .len = strlen("http://localhost:8000/imgfs/read?max_file=100&img_id=mure.jpg")};

    char out[256];
    int result;

    result = http_get_var(&url1, "res", out, sizeof(out));
    ck_assert_int_eq(result, 4);
    ck_assert_str_eq(out, "orig");

    result = http_get_var(&url1, "img_id", out, sizeof(out));
    ck_assert_int_eq(result, 8);
    ck_assert_str_eq(out, "mure.jpg");

    result = http_get_var(&url2, "max_file", out, sizeof(out));
    ck_assert_int_eq(result, 3);
    ck_assert_str_eq(out, "100");
}
END_TEST



START_TEST(test_http_get_var_not_found) {
    struct http_string url = {.val = "http://localhost:8000/imgfs/read?res=orig&img_id=mure.jpg",
            .len = strlen("http://localhost:8000/imgfs/read?res=orig&img_id=mure.jpg")};
    char out[256];
    int result = http_get_var(&url, "max_files", out, sizeof(out));
    //should return 0 if the parameter is not found (handout)
    ck_assert_int_eq(result, 0);
}
END_TEST



START_TEST(test_get_next_token) {
    struct http_string output;
    const char *remainder;

    char tempBuffer[1024]; // Ensure this is large enough for your tests

    //======================================================

    remainder = get_next_token("abcdefg", "de", &output);
    memcpy(tempBuffer, output.val, output.len);
    tempBuffer[output.len] = '\0';

    ck_assert_str_eq(tempBuffer, "abc");
    ck_assert_int_eq(output.len, 3);
    ck_assert_str_eq(remainder, "fg");

    //======================================================

    remainder = get_next_token("Content-Length: 0\r\nAccept: */*", ": ", &output);
    memcpy(tempBuffer, output.val, output.len);
    tempBuffer[output.len] = '\0';

    ck_assert_str_eq(tempBuffer, "Content-Length");
    ck_assert_int_eq(output.len, strlen("Content-Length"));
    ck_assert_str_eq(remainder, "0\r\nAccept: */*");
}
END_TEST



START_TEST(test_http_parse_headers) {
    struct http_message msg;
    const char *header_str = "Host: localhost:8000\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n";
    const char *result;

    result = http_parse_headers(header_str, &msg);
    ck_assert_int_eq(msg.num_headers, 3);
    ck_assert_str_eq(msg.headers[0].key.val, "Host");
    ck_assert_str_eq(msg.headers[0].value.val, "localhost:8000");
    ck_assert_str_eq(msg.headers[1].key.val, "User-Agent");
    ck_assert_str_eq(msg.headers[1].value.val, "curl/8.5.0");
    ck_assert_str_eq(msg.headers[2].key.val, "Accept");
    ck_assert_str_eq(msg.headers[2].value.val, "*/*");
    ck_assert_ptr_eq(result, header_str + strlen(header_str)); // Expect to point at the end of headers
}
END_TEST



START_TEST(test_http_parse_message_complete) {
    struct http_message msg;
    int content_len;
    const char *message = "Host: localhost:8000\r\nContent-Length: 10\r\n\r\n1234567890";
    int result = http_parse_message(message, strlen(message), &msg, &content_len);

    ck_assert_int_eq(result, 1); // Fully parsed message
    ck_assert_int_eq(content_len, 10);
    ck_assert_int_eq(msg.body.len, 10);
    ck_assert_str_eq(msg.body.val, "1234567890");
}
END_TEST



START_TEST(test_http_parse_message_incomplete_body) {
    struct http_message msg;
    int content_len;
    const char *message = "Host: localhost:8000\r\nContent-Length: 10\r\n\r\n1234";
    int result = http_parse_message(message, strlen(message), &msg, &content_len);

    ck_assert_int_eq(result, 0); // Incomplete body
    ck_assert_int_eq(content_len, 10);
    ck_assert_int_eq(msg.body.len, 0); // No body parsed
}
END_TEST



Suite* http_suite(void) {
    Suite *s = suite_create("HTTP Protocol Handling");

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_http_get_var_found);
    tcase_add_test(tc_core, test_http_get_var_not_found);
    tcase_add_test(tc_core, test_http_get_var_found);
    tcase_add_test(tc_core, test_http_get_var_not_found);
    tcase_add_test(tc_core, test_get_next_token);
    tcase_add_test(tc_core, test_http_parse_headers);
    tcase_add_test(tc_core, test_http_parse_message_complete);
    tcase_add_test(tc_core, test_http_parse_message_incomplete_body);

    suite_add_tcase(s, tc_core);

    return s;
}



Suite *personal_week12_test()
{
    Suite *s = suite_create("Tests implementation for week 12 (personal tests)");

    Add_Test(s, test_http_match_uri_prefix);
    Add_Test(s, test_http_match_verb_exact);
    Add_Test(s, test_http_get_var_found);
    Add_Test(s, test_http_get_var_not_found);
    Add_Test(s, test_get_next_token);
    Add_Test(s, test_http_parse_headers);
    Add_Test(s, test_http_parse_message_complete);
    Add_Test(s, test_http_parse_message_incomplete_body);

    return s;
}

TEST_SUITE(personal_week12_test)

