#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER "telehack.com"
#define PORT "1337"
#define COMMAND "figlet"
#define PROMPT "\r\n."

#define FONT_MAX_SZ 32
#define TEXT_MAX_SZ 1024
#define SEND_BUF_SZ 2048
#define RECV_BUF_SZ 2048
#define PROMPT_BUF_SZ 2048
#define PROMPT_SZ 3

void print_help(void) {
    printf("NAME:\n"
           "    telehack telnet client\n"
           "        returns text processed by 'figlet' command of telehack.com\n"
           "        server\n"
           "OPTIONS:\n"
           "    -f, --font\n"
           "        Font that should be applied to passed text\n"
           "    -t, --text"
           "        Text for processing"
           "    -h, --help\n"
           "        Print help\n");
}

void wait_for_cmd_prompt(int sock_fd) {
    ssize_t len = 0, received = 0;
    char prompt_buf[PROMPT_BUF_SZ] = {0};

    while ((received = recv(sock_fd, prompt_buf + len, 1, 0)) > 0) {
        len += received;

        if (len >= PROMPT_SZ && (0 == strncmp(prompt_buf + len - PROMPT_SZ, PROMPT, PROMPT_SZ))) {
            break; // while
        }
    }
}

void print_text(const char* font, const char* text) {
    struct addrinfo hint, *servinfo, *iter;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    int ainfo = getaddrinfo(SERVER, PORT, &hint, &servinfo);
    if (0 != ainfo) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ainfo));
        exit(EXIT_FAILURE);
    }

    printf("connecting to %s, port %s...\n", SERVER, PORT);
    int sock_fd;
    for (iter = servinfo; iter != NULL; iter = iter->ai_next) {
        sock_fd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
        if (-1 == sock_fd) {
            perror("socket");
            continue; // try next one
        }

        if (-1 == connect(sock_fd, iter->ai_addr, iter->ai_addrlen)) {
            close(sock_fd);
            perror("connect");
            continue; // try next one
        }
        break; // connected to the server
    }

    if (iter == NULL) {
        fprintf(stderr, "failed to connect\n");
        exit(EXIT_FAILURE);
    }

    char ip_addr[INET_ADDRSTRLEN];
    inet_ntop(iter->ai_family, iter->ai_addr, ip_addr, sizeof(ip_addr));
    freeaddrinfo(servinfo);
    printf("connected to %s\n", ip_addr);

#define GRACEFULLY_CLOSE(fd)                                                                                           \
    shutdown((fd), SHUT_RDWR);                                                                                         \
    close((fd));

    wait_for_cmd_prompt(sock_fd);

    char figlet_cmd[SEND_BUF_SZ];
    snprintf(figlet_cmd, SEND_BUF_SZ, "%s --%s %s\n\r.", COMMAND, font, text);

    if (send(sock_fd, figlet_cmd, strlen(figlet_cmd) + 1, 0) < 0) {
        perror("send");
        GRACEFULLY_CLOSE(sock_fd);
        exit(EXIT_FAILURE);
    }

    char* recv_buf = malloc(RECV_BUF_SZ);
    if (NULL == recv_buf) {
        perror("malloc");
        GRACEFULLY_CLOSE(sock_fd);
        exit(EXIT_FAILURE);
    }
    memset(recv_buf, 0, RECV_BUF_SZ);

    ssize_t len = 0, received = 0;
    while ((received = recv(sock_fd, recv_buf + len, 1, 0)) > 0) {
        len += received;
        if (len > RECV_BUF_SZ - 1) {
            perror("recv buf overflow");
            free(recv_buf);
            GRACEFULLY_CLOSE(sock_fd);
            exit(EXIT_FAILURE);
        }

        if (len >= PROMPT_SZ && (0 == strncmp(recv_buf + len - PROMPT_SZ, PROMPT, PROMPT_SZ))) {
            break; // while
        }
    }
    if (received < 0) {
        perror("recv");
        free(recv_buf);
        GRACEFULLY_CLOSE(sock_fd);
        exit(EXIT_FAILURE);
    }
    recv_buf[len - PROMPT_SZ - 1] = '\0';          // cut off next cmd prompt
    printf("%s\n", recv_buf + strlen(figlet_cmd)); // cut off figlet cmd

    free(recv_buf);
    GRACEFULLY_CLOSE(sock_fd);
}

int main(int argc, char** argv) {
    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"font", required_argument, 0, 'f'},
        {"text", required_argument, 0, 't'},
        {0, 0, 0, 0},
    };
    char* font = NULL;
    char* text = NULL;
    int opt_char = 0;

    while (-1 != (opt_char = getopt_long(argc, argv, ":f:t:h", opts, NULL))) {
        switch (opt_char) {
            case 'f': {
                font = optarg;
                break;
            }
            case 't': {
                text = optarg;
                break;
            }
            case 'h': {
                print_help();
                exit(EXIT_SUCCESS);
            }
            case '?': {
                fprintf(stderr, "unknown option: '%c'\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
            case ':': {
                fprintf(stderr, "missing arg for: '%c'\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
        }
    }
    if (strlen(font) > FONT_MAX_SZ) {
        fprintf(stderr, "max font size exceeded, exiting...\n");
        exit(EXIT_FAILURE);
    }
    if (strlen(text) > TEXT_MAX_SZ) {
        fprintf(stderr, "max text size exceeded, exiting...\n");
        exit(EXIT_FAILURE);
    }
    print_text(font, text);
    exit(EXIT_SUCCESS);
}
