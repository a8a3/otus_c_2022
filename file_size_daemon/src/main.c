#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <bits/getopt_core.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h> // getrlimit
#include <sys/socket.h>
#include <sys/stat.h> // stat
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>

#include <unistd.h>

#include <config.h>

#define SERV_NAME "fszd.socket"

#define PERR_EXIT(msg)                                                                                                 \
    perror(msg);                                                                                                       \
    exit(EXIT_FAILURE);

void print_help(void) {
    printf("NAME:\n"
           "    fszd\n"
           "        File SiZe Daemon\n"
           "OPTIONS:\n"
           "    -c, --config\n"
           "        Configuration file path"
           "    -N, --no-daemon\n"
           "        Run without demonization\n"
           "    -h, --help\n"
           "        Print help\n");
}

static volatile sig_atomic_t int_received = 0;
static volatile sig_atomic_t hup_received = 0;
char* cfg_file_name = NULL;

void sig_handler(int sig) {
    if (SIGINT == sig) {
        int_received = 1;
        return;
    }
    if (SIGHUP == sig) {
        hup_received = 1;
        return;
    }
}

long get_file_size(const char* file_name) {
    struct stat st;
    if (-1 == stat(file_name, &st)) {
        fprintf(stderr, "unable to read file: %s", file_name);
    }
    return st.st_size;
}

void run(void) {
    // TODO:
    // - check memory leaks

    int listener = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener < 0) {
        syslog(LOG_ERR, "listen socker err");
        return;
    }

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SERV_NAME);

    if (bind(listener, (struct sockaddr*)&server, sizeof(struct sockaddr_un)) < 0) {
        close(listener);
        syslog(LOG_ERR, "listen socket bind err");
        return;
    }

    if (listen(listener, 8) < 0) {
        close(listener);
        unlink(SERV_NAME);
        syslog(LOG_ERR, "listen err");
        return;
    }

    syslog(LOG_INFO, "start listeng on %s", server.sun_path);
    int client = 0;
    char buf[1024];
    while (0 == int_received) {
        if (-1 == (client = accept(listener, 0, 0))) {
            if (EAGAIN == errno || EWOULDBLOCK == errno) {
                continue; // while
            }
            syslog(LOG_ERR, "client accep err");
            break; // while
        }
        const char* file_path = cfg_get_file_name();
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s size is [%ld] bytes\n", file_path, get_file_size(file_path));

        send(client, buf, strlen(buf), 0);
        close(client);

        if (1 == hup_received) {
            hup_received = 0;
            read_cfg(cfg_file_name);
        }
    }
    close(listener);
    unlink(SERV_NAME);
}

// by W.Richard Stevens. Advanced Programmming in the UNIX Environment
void daemonize(void) {
    umask(0); // clear file creation mask

    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) { // get max num of file descriptors
        PERR_EXIT("getrlimit");
    }

    // 1st fork
    // it does:
    // - if run by shell, it makes the shell think that the command is done
    // - child inherits PGID of the parent but gets a new PID, it guarantees the child is not a process group
    //   leader(prerequisite for the setsid call below)
    pid_t pid;
    if ((pid = fork()) < 0) {
        PERR_EXIT("1st fork failed");
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    setsid(); // become a session leader to lose controlling TTY

    // 2nd fork
    // after 2nd fork call daemon is no longer a session leader
    if ((pid = fork()) < 0) {
        PERR_EXIT("2nd fork failed");
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // close all open FDs
    if (RLIM_INFINITY == rl.rlim_max) {
        rl.rlim_max = 1024;
    }

    for (rlim_t i = 0; i < rl.rlim_max; ++i) {
        close(i);
    }

    // attach FDs 0, 1, 2 to /dev/null
    int daemon_in = open("/dev/null", O_RDWR);
    assert(0 == daemon_in);
    dup(0); // daemon out
    dup(0); // daemon err
}

int main(int argc, char** argv) {

    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"config", required_argument, 0, 'c'},
        {"no-daemon", no_argument, 0, 'N'},
        {0, 0, 0, 0},
    };
    int opt_char = 0;
    bool no_daemon_mode = false;

    while (-1 != (opt_char = getopt_long(argc, argv, ":hNc:", opts, NULL))) {
        switch (opt_char) {
            case 'h': {
                print_help();
                exit(EXIT_SUCCESS);
            }
            case 'N': {
                no_daemon_mode = true;
                break;
            }
            case 'c': {
                cfg_file_name = optarg;
                break;
            }
            case '?': {
                fprintf(stderr, "unknown option: '%c'\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!no_daemon_mode) {
        daemonize();
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_handler;
    if (-1 == sigaction(SIGHUP, &sa, NULL) || -1 == sigaction(SIGINT, &sa, NULL)) {
        PERR_EXIT("sigaction");
    }

    assert(argc > 0);
    openlog(argv[0], LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "started");
    read_cfg(cfg_file_name);
    run();
    syslog(LOG_INFO, "stopped");
    closelog();
    exit(EXIT_SUCCESS);
}
