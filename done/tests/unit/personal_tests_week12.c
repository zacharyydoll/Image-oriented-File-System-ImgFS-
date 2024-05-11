#include "http_prot.h"
#include <check.h>
#include "test.h"

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



Suite* http_suite(void) {
    Suite *s = suite_create("HTTP Protocol Handling");

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_http_get_var_found);
    tcase_add_test(tc_core, test_http_get_var_not_found);
    tcase_add_test(tc_core, test_http_get_var_found);
    tcase_add_test(tc_core, test_http_get_var_not_found);
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

    return s;
}

TEST_SUITE(personal_week12_test)

