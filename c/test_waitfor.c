#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>

#define main run_main
#include "wait-for-it.c"
#undef main

void test_parse_hostport() {
    char *host, *port;
    parse_hostport("hello.world:blabla", &host, &port);
    CU_ASSERT_STRING_EQUAL("hello.world", host);
    CU_ASSERT_STRING_EQUAL("blabla", port);
    free(host);
    free(port);

    parse_hostport("hello.world2", &host, &port);
    CU_ASSERT_STRING_EQUAL("hello.world2", host);
    CU_ASSERT_EQUAL(NULL, port);
    free(host);
}

void test_check_resolve() {
    CU_ASSERT_EQUAL(0, check_resolve("localhost"));
    CU_ASSERT_NOT_EQUAL(0, check_resolve("hello-nonexistent:2000"));
}

int main(void) {
    CU_pSuite suite;
    CU_initialize_registry();
    suite = CU_add_suite("waitfor Test", NULL, NULL);
    CU_add_test(suite, "test_parse_hostport", test_parse_hostport);
    CU_add_test(suite, "test_check_resolve", test_check_resolve);
    CU_basic_run_tests();
    int ret = CU_get_number_of_failures();
    CU_cleanup_registry();
    return ret;
}
