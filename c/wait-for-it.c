#include <arpa/inet.h>
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
        "        --dns        dns resolve(no connect)\n"
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

int check_dns(char *hostport) {
    // printf("check dns %s\n", hostport);
    struct addrinfo *res;
    char *nodename, *servname;
    parse_hostport(hostport, &nodename, &servname);
    // printf("dns %s port=%s\n", nodename, servname);
    int r = getaddrinfo(nodename, servname, NULL, &res);
    freeaddrinfo(res);
    free(nodename);
    if (servname != NULL) {
        free(servname);
    }
    if (r != 0) {
        // printf("error(%d) %s\n", r, gai_strerror(r));
        return -1;
    }
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

int check_connect(char *hostport) {
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
        int r = connect(s, i->ai_addr, i->ai_addrlen);
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
        {"dns", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0},
    };
    int quiet = 0;
    int strict = 0;
    int timeout = 30;
    int dns = 0;
    float interval = 1.0;
    char ch;
    while ((ch = getopt_long(argc, argv, "qst:i:hV", longopts, NULL)) != -1) {
        switch (ch) {
            case 'q':
                quiet = 1;
                break;
            case 's':
                strict = 1;
                break;
            case 'd':
                dns = 1;
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
    time_t timelimit = time(NULL) + timeout;
    char *hostport = *argv;
    if (hostport == NULL) {
        show_usage();
        return 0;
    }
    argv++;
    char **command = argv;
    if (!quiet) {
        printf(
            "quiet=%d, strict=%d, timeout=%d, interval=%f, dns=%d, "
            "hostport=%s\n",
            quiet, strict, timeout, interval, dns, hostport);
    }
    time_t start = time(NULL);
    while (1) {
        if (dns) {
            if (!check_dns(hostport)) {
                if (!quiet) {
                    printf("resolved after %ld sec.\n", time(NULL) - start);
                }
                break;
            }
        } else {
            if (!check_connect(hostport)) {
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
