#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>

#define main do_main
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

int get_listen_socket(char *hostport, size_t hostport_len) {
    int s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        perror("socket");
        return s;
    }
    struct sockaddr addr;
    memset(&addr, 0, sizeof(addr));
    addr.sa_family = AF_INET;
    int r1 = bind(s, &addr, sizeof(addr));
    if (r1 < 0) {
        perror("bind");
        return r1;
    }
    printf("addr family=%d, len=%d\n", addr.sa_family, addr.sa_len);
    int r2 = listen(s, 10);
    if (r2 < 0) {
        perror("listen");
        return r2;
    }
    struct sockaddr saddr;
    unsigned int socklen = sizeof(saddr);
    int r3 = getsockname(s, &saddr, &socklen);
    if (r3 < 0) {
        perror("getsockname");
        return r3;
    }
    ip2str(&saddr, hostport, hostport_len);
    return s;
}

void test_check_connect() {
    char hostport[1024];
    int sock = get_listen_socket(hostport, sizeof(hostport));
    if (sock < 0) {
        return;
    }
    CU_ASSERT_NOT_EQUAL(-1, sock);
    CU_ASSERT_EQUAL(0, check_connect(hostport));
    close(sock);
    CU_ASSERT_NOT_EQUAL(0, check_connect(hostport));
    CU_ASSERT_NOT_EQUAL(0, check_connect("hello-nonexistent:2000"));
}

void test_main_connect() {
    char hostport[1024];
    int sock = get_listen_socket(hostport, sizeof(hostport));
    if (sock < 0) {
        return;
    }
    CU_ASSERT_NOT_EQUAL(-1, sock);
    // localhost:port -t 1
    char *args[5];
    args[0] = "test";
    args[1] = hostport;
    args[2] = "-t";
    args[3] = "1";
    args[4] = NULL;
    CU_ASSERT_EQUAL(0, do_main(4, args));
    close(sock);
}

void test_main_resolve() {
    // localhost -t 1 --resolve
    char *args[6];
    args[0] = "test";
    args[1] = "localhost";
    args[2] = "-t";
    args[3] = "1";
    args[4] = "--resolve";
    args[5] = NULL;
    CU_ASSERT_EQUAL(0, do_main(5, args));
}

void test_main_resolve_fail() {
    // non-existent:port -t 1 --resolve
    char *args[6];
    args[0] = "test";
    args[1] = "non-existent:port";
    args[2] = "-t";
    args[3] = "1";
    args[4] = "--resolve";
    args[5] = NULL;
    CU_ASSERT_EQUAL(1, do_main(5, args));
}

int clearopt() {
    optreset = 1;
    return 0;
}

int main(void) {
    CU_pSuite suite;
    CU_initialize_registry();
    suite = CU_add_suite("waitfor Test", &clearopt, NULL);
    CU_add_test(suite, "test_parse_hostport", test_parse_hostport);
    CU_add_test(suite, "test_check_resolve", test_check_resolve);
    CU_add_test(suite, "test_check_connect", test_check_connect);
    CU_add_test(suite, "test_main_connect", test_main_connect);
    CU_add_test(suite, "test_main_resolve", test_main_resolve);
    // CU_add_test(suite, "test_main_resolve_fail", test_main_resolve_fail);
    CU_basic_run_tests();
    int ret = CU_get_number_of_failures();
    CU_cleanup_registry();
    return ret;
}
