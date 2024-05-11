#include "http_prot.h"
#include <check.h>
#include "test.h"

#define ck_assert_http_str_eq(str, expect) \
    ck_assert_msg(strncmp(str.val, expect, str.len) == 0 && strlen(expect) == str.len, \
                  "Expected '%s', but got '%.*s'", expect, (int)str.len, str.val)

START_TEST(test_http_match_uri_prefix)
{
    struct http_message msg1 = {.uri = {"https://localhost:8000/imgfs/read?res=orig&img_id=mure.jpg", 55}};
    struct http_message msg2 = {.uri = {"/universal/resource/identifier", 55}};


    // ===========================================================

    ck_assert(http_match_uri(&msg1, "https://") == 1);
    ck_assert(http_match_uri(&msg1, "https://localhost:8000/") == 1);
    ck_assert(http_match_uri(&msg1, "https://localhost:8000/imgfs") == 1);

    ck_assert(http_match_uri(&msg1, "http://") == 0);

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

Suite *personal_week12_test()
{
    Suite *s = suite_create("Tests implementation for week 12 (personal tests)");

    Add_Test(s, test_http_match_uri_prefix);
    Add_Test(s, test_http_match_verb_exact);

    return s;
}

TEST_SUITE(personal_week12_test)

