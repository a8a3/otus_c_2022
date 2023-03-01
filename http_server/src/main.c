#define _POSIX_C_SOURCE 200809L // NOLINT

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CONN 32
#define MAX_EVENTS 64
#define FULL_FILE_NAME_SZ 256
#define READ_SZ 4096 // assumed server doesn't expect more that 4K bytes request
#define MIN(a, b) (a) < (b) ? (a) : (b)
#define DEFAULT_DIR "."
#define DEFAULT_ADDR "127.0.0.1:8080"

void print_help(void) {
    printf("NAME:\n"
           "    http_srv\n"
           "        Files sharing HTTP- server\n"
           "OPTIONS:\n"
           "    -d, --directory\n"
           "        Directory with files to share\n"
           "    -a, --address\n"
           "        Listening address in <ip address>:<port> format\n"
           "    -h, --help\n"
           "        Print help\n");
}

bool init_ip_addr(const char* addr_str, struct sockaddr_in* addr) {
#define IP_BUF_SZ 16
    char ip[IP_BUF_SZ] = {0};

    // expected format example: 127.0.0.1:8080
    char* colon_ptr = strchr(addr_str, ':');
    if (NULL == colon_ptr) {
        return false;
    }

    int port = atoi(colon_ptr + 1);
    if (0 == port) {
        return false;
    }

    strncpy(ip, addr_str, MIN(IP_BUF_SZ - 1, colon_ptr - addr_str));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_port = htons(port);
    return true;
}

bool set_nonblocking(int sock_fd) { return -1 != fcntl(sock_fd, F_SETFD, fcntl(sock_fd, F_GETFD, 0) | O_NONBLOCK); }

bool epoll_add(int epoll_fd, int fd, uint32_t events) {
    static struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return -1 != epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

char* get_requested_file_name(const char* buf) {
    // expected format of custom header with file name:
    // file:<file_name>
#define FILE_HDR "file:"
#define FILE_NAME_BUF_SZ 256
    char* file_begin = strstr(buf, FILE_HDR);
    if (!file_begin) {
        perror("no file header passed");
        return NULL;
    }
    file_begin += strlen(FILE_HDR);
    char* file_end = strchr(file_begin, '\r');
    int file_name_len = MIN(FILE_NAME_BUF_SZ - 1, file_end - file_begin);

    static char file_name_buf[FILE_NAME_BUF_SZ];
    strncpy(file_name_buf, file_begin, file_name_len);
    file_name_buf[file_name_len] = '\0';
    return file_name_buf;
}

//bool is_file_exists(const char* file_name) {
//    struct stat st;
//    return (0 == stat(file_name, &st));
//}

int errno_2http_status(void) {
    switch(errno) {
        case 2: return 404;  // no file or directory
        case 13: return 403; // permission denied 
    }
    return -1;
}


void run(const char* dir_name, const char* listen_addr) {
    struct sockaddr_in srv_addr, client_addr = {0};
    if (!init_ip_addr(listen_addr, &srv_addr)) {
        perror("bad listening address specified");
        exit(EXIT_FAILURE);
    }

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (0 != bind(listener, (struct sockaddr*)&srv_addr, sizeof(srv_addr))) {
        close(listener);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (!set_nonblocking(listener)) {
        close(listener);
        perror("set_nonblocking");
        exit(EXIT_FAILURE);
    }

    listen(listener, MAX_CONN);
    printf("start listening on %s...\n", listen_addr);

    int epoll_fd = epoll_create1(0);
    if (-1 == epoll_fd) {
        close(listener);
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    if (!epoll_add(epoll_fd, listener, EPOLLIN | EPOLLOUT | EPOLLET)) {
        close(listener);
        perror("epoll_ctl_add");
        exit(EXIT_FAILURE);
    }
    int fd_ready;
    struct epoll_event events[MAX_EVENTS];
    socklen_t client_addr_sz = sizeof(client_addr);
    int client;
    ssize_t rd;
    char full_file_name[256];
    char buf[READ_SZ];

    for (;;) {
        fd_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < fd_ready; ++i) {

            // check if new client wants to connect
            if (listener == events[i].data.fd) {
                client = accept(listener, (struct sockaddr*)&client_addr, &client_addr_sz);
                inet_ntop(AF_INET, (char*)&client_addr.sin_addr, buf, sizeof(client_addr));
                printf("new client accepted: %s:%d\n", buf, ntohs(client_addr.sin_port));
                if (!set_nonblocking(client)) {
                    perror("unable to make non-blocking client");
                    continue;
                }

                if (!epoll_add(epoll_fd, client, EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP)) {
                    perror("unable to add client to epoll");
                    continue;
                }
                // client's incoming request
            } else if (events[i].events & EPOLLIN) {
                for (;;) {
                    memset(buf, 0, READ_SZ);
                    rd = read(events[i].data.fd, buf, READ_SZ - 1);
                    if (rd <= 0) {
                        break; // for
                    }
                    buf[READ_SZ - 1] = '\0';
                    printf("received from client:\n%s\n", buf);

                    char* requested_file = get_requested_file_name(buf);
                    memset(full_file_name, 0, sizeof(full_file_name));
                    snprintf(full_file_name, FULL_FILE_NAME_SZ, "%s/%s", dir_name, requested_file);

                    printf("requested file: [%s]\n", full_file_name);

                    // TODO: check:
                    // 1. if file exists
                    // 2. if proc has permissions to read
                    // 3. use stat call for that...
//                    FILE* fd = fopen(full_file_name, "r"); 
//                    if (!is_file_exists(full_file_name)) {
//                        // TODO: no file response
//                        printf("no file: %s\n", full_file_name);
//                        continue;
//                    }

                    // TODO: read in binary mode
                    int fd = open(full_file_name, O_RDONLY);
                    if (-1 == fd) {
                        perror("open");
                        printf("errno: %d\n", errno);
                        
                        int http_status = errno_2http_status();
                        if (-1 == http_status) {
                            // send "internal server error?"
                        }

                        continue;
                    }
                    
                    printf("%s is opened\n", full_file_name);
                    close(fd);
                }
            } else {
                printf("unexpected event: %u", events[i].events);
            }

            // check if existing client wants to disconnect
            if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
                printf("client disconnected\n");
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                close(events[i].data.fd);
            }
        }
    }
    close(epoll_fd);
    close(listener);
}

int main(int argc, char** argv) {
    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"directory", required_argument, 0, 'd'},
        {"address", required_argument, 0, 'a'},
        {0, 0, 0, 0},
    };
    char* dir_name = DEFAULT_ADDR;
    char* listen_addr = DEFAULT_DIR;
    int opt_char = 0;

    while (-1 != (opt_char = getopt_long(argc, argv, ":d:a:h", opts, NULL))) {
        switch (opt_char) {
            case 'd': {
                dir_name = optarg;
                break;
            }
            case 'a': {
                listen_addr = optarg;
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
    run(dir_name, listen_addr);
    exit(EXIT_SUCCESS);
}
