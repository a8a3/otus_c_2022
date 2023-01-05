#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <bits/getopt_core.h>

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h> // getrlimit
#include <sys/stat.h>     // stat
#include <syslog.h>

#include <unistd.h>

#define PERR_EXIT(msg)                                                                                                 \
    perror(msg);                                                                                                       \
    exit(EXIT_FAILURE);

void print_help(void) {
    printf("NAME:\n"
           "\tfszd\n"
           "\t\tFile SiZe Daemon\n"
           "OPTIONS:\n"
           "\t-d, --daemon\n"
           "\t\tRun in daemon mode\n"
           "\t-f, --file\n"
           "\t\tIn normal mode specifies the name of file to print its size\n"
           "\t-h, --help\n"
           "\t\tPrint help\n"
           "USAGE:\n"
           "\tfszd -d\n"
           "\tfszd -f file_name\n");
}

void print_file_size(const char* file_name) {
    struct stat st;
    if (-1 == stat(file_name, &st)) {
        PERR_EXIT("stat");
    }
    printf("%s size is [%ld] bytes\n", file_name, st.st_size);
}

void do_work(void) { sleep(60); }

// by W.Richard Stevens. Advanced Programmming in the UNIX Environment
void daemonize(const char* proc_name) {
    umask(0); // clear file creation mask(not going to create new files)

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
        PERR_EXIT("1st fork");
    }

    if (0 != pid) { // return if parent
        return;
    }
    setsid(); // become a session leader to lose controlling TTY

    // ensure future opens won't allocate controlling TTYs
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        PERR_EXIT("sigaction");
    }

    // 2nd fork
    // after fork call daemon is no longer a session leader
    if ((pid = fork() < 0)) {
        PERR_EXIT("2nd fork");
    }

    if (0 != pid) { // return if parent
        return;
    }

    // change current directory to the root- prevent file systems from being unmounted
    if (chdir("/") < 0) {
        PERR_EXIT("chdir");
    }

    // close all open FDs
    if (RLIM_INFINITY == rl.rlim_max) {
        rl.rlim_max = 1024;
    }

    for (rlim_t i = 0; i < rl.rlim_max; ++i) {
        close(i);
    }

    // attach FDs 0, 1, 2 to /dev/null
    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);

    // initialize system log
    openlog(proc_name, LOG_CONS, LOG_DAEMON);
    if (0 != fd0 || 1 != fd1 || 2 != fd2) {
        syslog(LOG_ERR, "unexpected file descriptors: %d, %d, %d", fd0, fd1, fd2);
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "%s started", proc_name);
    do_work();
    syslog(LOG_INFO, "%s stopped", proc_name);
    closelog();
}

int main(int argc, char** argv) {

    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"file", required_argument, 0, 'f'},
        {"daemon", no_argument, 0, 'd'},
        {0, 0, 0, 0},
    };
    int opt_char = 0;
    bool daemon_mode = false;
    char* file_name = NULL;

    while (-1 != (opt_char = getopt_long(argc, argv, ":hdf:", opts, NULL))) {
        switch (opt_char) {
            case 'h': {
                print_help();
                exit(EXIT_SUCCESS);
            }
            case 'd': {
                daemon_mode = true;
                break;
            }
            case 'f': {
                file_name = optarg;
                break;
            }
            case '?': {
                fprintf(stderr, "unknown option: '%c'\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
        }
    }

    if (daemon_mode) {
        assert(argc > 0);
        daemonize(argv[0]);
    } else {
        if (NULL == file_name) {
            fprintf(stderr, "no file name specified\nexit\n");
            exit(EXIT_FAILURE);
        }
        print_file_size(file_name);
    }
    exit(EXIT_SUCCESS);
}
