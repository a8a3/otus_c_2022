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
#include <sys/sendfile.h>
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
#define RESPONSE_HDR_SZ 1024

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
    // expected format example: 127.0.0.1:8080
    char ip[16] = {0};
    char* colon_ptr = strchr(addr_str, ':');
    if (NULL == colon_ptr) {
        return false;
    }

    int port = atoi(colon_ptr + 1);
    if (0 == port) {
        return false;
    }

    strncpy(ip, addr_str, MIN((long)sizeof(ip) - 1, colon_ptr - addr_str));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_port = htons(port);
    return true;
}

bool set_nonblocking(int sock_fd) {
    int fd_get_res = fcntl(sock_fd, F_GETFD, 0);
    if (-1 == fd_get_res) {
        return false;
    }
    return -1 != fcntl(sock_fd, F_SETFD, fd_get_res | O_NONBLOCK);
}

bool epoll_add(int epoll_fd, int fd, uint32_t events) {
    static struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return -1 != epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

char* get_requested_file_name(const char* buf) {
    char* file_begin = strchr(buf, '/');
    if (!file_begin) {
        fprintf(stderr, "no file name passed\n");
        return NULL;
    }
    char* file_end = strchr(file_begin, ' ');
    if (!file_end) {
        fprintf(stderr, "no file name passed\n");
        return NULL;
    }
    file_begin += 1;
    if (file_begin == file_end) {
        fprintf(stderr, "no file name passed\n");
        return NULL;
    }
    static char file_name_buf[256];
    int file_name_len = MIN((long)sizeof(file_name_buf) - 1, file_end - file_begin);

    strncpy(file_name_buf, file_begin, sizeof(file_name_buf));
    file_name_buf[file_name_len] = '\0';
    return file_name_buf;
}

typedef struct {
    int code;
    const char* text;
} http_status;

http_status* errno_2http_status(int err) {
    static http_status status;
    switch (err) {
        case 0:
            status.code = 200;
            status.text = "OK";
            break;
        case 2:
            status.code = 404;
            status.text = "Not found";
            break;
        case 13:
            status.code = 403;
            status.text = "Forbidden";
            break;
        default:
            status.code = 500;
            status.text = "Internal server error";
    }
    return &status;
}

void send_http_err(http_status* status, int client) {
    char response_hdr_buf[RESPONSE_HDR_SZ];
    const char* hdr_template = "HTTP/1.1 %d %s\r\n"
                               "Server: FileSrv\r\n\r\n";
    int response_sz = snprintf(response_hdr_buf, RESPONSE_HDR_SZ, hdr_template, status->code, status->text);

    ssize_t bytes_total = 0, bytes_current = 0;
    do {
        bytes_current = write(client, response_hdr_buf + bytes_total, response_sz - bytes_total);
        if (-1 == bytes_current) {
            perror("write http err");
            return;
        }
        bytes_total += bytes_current;
    } while (bytes_total < response_sz);
}

bool send_file(int fd, int client) {
    struct stat fd_stat;
    int res = fstat(fd, &fd_stat);
    if (-1 == res) {
        perror("fstat");
        send_http_err(errno_2http_status(-1), client);
        return false;
    }
    intmax_t file_sz = fd_stat.st_size;
    printf("file size: %jd bytes\n", file_sz);

    char response_hdr_buf[RESPONSE_HDR_SZ];
    const char* hdr_template = "HTTP/1.1 %d %s\r\n"
                               "Server: FileSrv\r\n"
                               "Content-Length: %jd\r\n"
                               "Connection: close\r\n\r\n";

    http_status* status = errno_2http_status(0);
    int headers_sz = snprintf(response_hdr_buf, RESPONSE_HDR_SZ, hdr_template, status->code, status->text, file_sz);

    ssize_t bytes_total = 0, bytes_current = 0;
    do {
        bytes_current = write(client, response_hdr_buf + bytes_total, headers_sz - bytes_total);
        if (-1 == bytes_current) {
            perror("write headers");
            return false;
        }
        bytes_total += bytes_current;
    } while (bytes_total < headers_sz);

    bytes_total = 0;
    do {
        bytes_current = sendfile(client, fd, &bytes_total, file_sz - bytes_total);
        if (-1 == bytes_current) {
            perror("write file");
            return false;
        }
        bytes_total += bytes_current;
    } while (bytes_total < file_sz);
    return true;
}

void run(const char* dir_name, const char* listen_addr) {
    struct sockaddr_in srv_addr, client_addr = {0};
    if (!init_ip_addr(listen_addr, &srv_addr)) {
        perror("bad listen address specified");
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
                    printf("client request:\n%s\n", buf);

                    char* requested_file = get_requested_file_name(buf);
                    if (NULL == requested_file) {
                        // no file name passed, close connection
                        send_http_err(errno_2http_status(2), events[i].data.fd);
                        close(events[i].data.fd);
                        fprintf(stderr, "disconnect client...\n");
                        continue;
                    }
                    memset(full_file_name, 0, sizeof(full_file_name));
                    snprintf(full_file_name, FULL_FILE_NAME_SZ, "%s/%s", dir_name, requested_file);

                    printf("requested file: \"%s\"\n", full_file_name);

                    int fd = open(full_file_name, O_RDONLY);
                    if (-1 == fd) {
                        int err_code = errno;
                        perror("open");
                        send_http_err(errno_2http_status(err_code), events[i].data.fd);
                        close(events[i].data.fd);
                        fprintf(stderr, "disconnect client...\n");
                        continue;
                    }
                    bool res = send_file(fd, events[i].data.fd);
                    close(fd);

                    if (res) {
                        printf("file has been sent\n");
                    } else {
                        send_http_err(errno_2http_status(-1), events[i].data.fd);
                        close(events[i].data.fd);
                        fprintf(stderr, "file sending error. disconnect client...\n");
                    }
                }
            } else {
                printf("unexpected event: %u", events[i].events);
            }

            // check if existing client wants to disconnect
            if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                close(events[i].data.fd);
                printf("client disconnected\n");
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
