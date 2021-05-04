#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#ifdef __GLIBC__
#warning "strlcat replacement"
int strlcat(char *restrict dst, const char *src, size_t len) {
    strncat(dst, src, len - strlen(dst));
    dst[len - 1] = '\0';
    return strlen(src);
}
#endif  // __GLIBC__

void show_usage() {
    printf(
        "USAGE:\n"
        "    wait-for-it [FLAGS] [OPTIONS] <hostport> [command]...\n\n"
        "FLAGS:\n"
        "        --resolve    check resolve(no connect)\n"
        "    -h, --help       Prints help information\n"
        "    -q, --quiet      Don't output any status messages\n"
        "    -s, --strict     Only execute subcommand if the test succeeds\n"
        "    -V, --version    Prints version information\n\n"
        "OPTIONS:\n"
        "    -i, --interval <interval>    interval second [default: 1.0]\n"
        "    -t, --timeout <timeout>      Timeout in seconds, zero for no "
        "timeout [default: 30]\n\n"
        "ARGS:\n"
        "    <hostport>      host:port\n"
        "    <command>...\n");
}

void show_version() { printf("wait-for-it 0.1.0\n"); }

void parse_hostport(char *hostport, char **nodename, char **servname) {
    *nodename = strdup(hostport);
    char *s = rindex(*nodename, ':');
    if (s != NULL) {
        *s = '\0';
        *servname = strdup(s + 1);
    } else {
        *servname = NULL;
    }
    return;
}

int check_resolve(char *hostport) {
    // printf("check resolve %s\n", hostport);
    struct addrinfo *res;
    char *nodename, *servname;
    parse_hostport(hostport, &nodename, &servname);
    // printf("resolve %s port=%s\n", nodename, servname);
    int r = getaddrinfo(nodename, servname, NULL, &res);
    free(nodename);
    if (servname != NULL) {
        free(servname);
    }
    if (r != 0) {
        // printf("error(%d) %s\n", r, gai_strerror(r));
        return -1;
    }
    freeaddrinfo(res);
    return 0;
}

char *ip2str(const struct sockaddr *sa, char *s, size_t maxlen) {
    char port[100];
    switch (sa->sa_family) {
        case AF_INET:
            inet_ntop(sa->sa_family, &(((struct sockaddr_in *)sa)->sin_addr), s,
                      maxlen);
            snprintf(port, sizeof(port), ":%d",
                     ntohs(((struct sockaddr_in *)sa)->sin_port));
            strlcat(s, port, maxlen);
            break;
        case AF_INET6:
            inet_ntop(sa->sa_family, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                      s, maxlen);
            snprintf(port, sizeof(port), ":%d",
                     ntohs(((struct sockaddr_in6 *)sa)->sin6_port));
            strlcat(s, port, maxlen);
            break;
        default:
            strncpy(s, "unknown", maxlen);
            return NULL;
    }
    return s;
}

int check_connect(char *hostport, float timeout_sec) {
    int ret = -1;
    struct addrinfo *res;
    struct addrinfo hint = {
        0,
        0,
        SOCK_STREAM,
        IPPROTO_TCP,
    };
    char *nodename, *servname;
    parse_hostport(hostport, &nodename, &servname);
    // printf("check connect %s:%s\n", nodename, servname);
    int r = getaddrinfo(nodename, servname, &hint, &res);
    free(nodename);
    if (servname != NULL) {
        free(servname);
    }
    if (r != 0) {
        // printf("error(%d) %s\n", r, gai_strerror(r));
        return -1;
    }
    for (struct addrinfo *i = res; i; i = i->ai_next) {
        // char buf[100];
        // printf("addr %s\n", ip2str(i->ai_addr, buf, sizeof(buf)));
        int s = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (s < 0) {
            continue;
        }
        struct timeval tv;
        tv.tv_sec = (int)(timeout_sec);
        tv.tv_usec = (int)(timeout_sec / 1000000.0);
        int r0 =
            setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
        if (r0 < 0) {
            perror("setsockopt");
        }
        int flag = fcntl(s, F_GETFL, NULL);
        if (flag < 0) {
            perror("fcntl(GETFL)");
        } else {
            flag |= O_NONBLOCK;
            int r1 = fcntl(s, F_SETFL, flag);
            if (r1 < 0) {
                perror("fcntl(SETFL)");
            }
        }
        int r = connect(s, i->ai_addr, i->ai_addrlen);
        if (r < 0 && errno == EINPROGRESS) {
            struct timeval tv;
            tv.tv_sec = (int)(timeout_sec);
            tv.tv_usec = (int)(timeout_sec / 1000000.0);
            fd_set sset, eset, rset;
            FD_ZERO(&sset);
            FD_ZERO(&rset);
            FD_ZERO(&eset);
            FD_SET(s, &sset);
            FD_SET(s, &rset);
            FD_SET(s, &eset);
            int res = select(s + 1, &rset, &sset, &eset, &tv);
            if (res > 0 && FD_ISSET(s, &sset) && !FD_ISSET(s, &eset) &&
                !FD_ISSET(s, &rset)) {
                // connected
                r = res;
            }
        }
        close(s);
        if (r < 0) {
            continue;
        }
        ret = 0;
        break;
    }
    freeaddrinfo(res);
    return ret;
}

void run_command(char **command, int quiet) {
    char *cmd = command[0];
    char **args = &command[0];
    if (cmd == NULL) {
        return;
    }
    if (!quiet) {
        printf("run command %s\n", cmd);
    }
    int r = execvp(cmd, args);
    if (r) {
        perror("exec failed\n");
    }
    return;
}

int main(int argc, char **argv) {
    struct option longopts[] = {
        {"quiet", no_argument, NULL, 'q'},
        {"strict", no_argument, NULL, 's'},
        {"timeout", required_argument, NULL, 't'},
        {"interval", required_argument, NULL, 'i'},
        {"resolve", no_argument, NULL, 'r'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0},
    };
    int quiet = 0;
    int strict = 0;
    int timeout = 30;
    int resolve = 0;
    float interval = 1.0;
    char ch;
    while ((ch = getopt_long(argc, argv, "qst:i:rhV", longopts, NULL)) != -1) {
        switch (ch) {
            case 'q':
                quiet = 1;
                break;
            case 's':
                strict = 1;
                break;
            case 'r':
                resolve = 1;
                break;
            case 't':
                timeout = strtol(optarg, NULL, 0);
                break;
            case 'i':
                interval = strtof(optarg, NULL);
                break;
            case 'V':
                show_version();
                return 0;
            default:
                show_usage();
                return 0;
        }
    }
    argc -= optind;
    argv += optind;
    char *hostport = *argv;
    if (hostport == NULL) {
        show_usage();
        return 0;
    }
    argv++;
    char **command = argv;
    if (!quiet) {
        printf(
            "quiet=%d, strict=%d, timeout=%d, interval=%f, resolve=%d, "
            "hostport=%s\n",
            quiet, strict, timeout, interval, resolve, hostport);
    }
    time_t start = time(NULL);
    while (1) {
        if (resolve) {
            if (!check_resolve(hostport)) {
                if (!quiet) {
                    printf("resolved after %ld sec.\n", time(NULL) - start);
                }
                break;
            }
        } else {
            if (!check_connect(hostport, interval / 2)) {
                if (!quiet) {
                    printf("connected after %ld sec.\n", time(NULL) - start);
                }
                break;
            }
        }
        if (timeout != 0 && time(NULL) - start > timeout) {
            if (!quiet) {
                printf("connect failed\n");
            }
            if (!strict) {
                run_command(command, quiet);
            }
            return 1;
        }
        usleep(interval * 1000000.0);
    }
    run_command(command, quiet);
    return 0;
}
