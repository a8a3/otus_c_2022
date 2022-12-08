#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "logger.h"

#define NANOS_IN_MICROS 1000
#define TIME_BUF_SZ 32

struct logger {
    FILE* fd;
};

static char* current_time_str(char* buf, size_t buf_sz) {
    struct timespec ts;
    if (-1 == clock_gettime(CLOCK_REALTIME, &ts)) {
        return NULL;
    }
    struct tm t;
    localtime_r(&ts.tv_sec, &t);
    snprintf(buf, buf_sz, "%02d:%02d:%02d.%06ld", t.tm_hour, t.tm_min, t.tm_sec, ts.tv_nsec / NANOS_IN_MICROS);
    return buf;
}

extern char* get_current_time_us() {
    static char buf[TIME_BUF_SZ];
    return current_time_str(buf, sizeof buf);
}

extern FILE* logger_get_fd(LOGGER l) { return l->fd; }

extern void logger_print(LOGGER l, const char* format, ...) {
    va_list args;
    va_start(args, format);
    // print out payload
    vfprintf(l->fd, format, args);
    va_end(args);
}

static char* severities_arr[] = {"UNK", "INF", "WRN", "ERR", "DBG"};
extern char* severity_as_str(severity s) {
    assert(s < last);
    return severities_arr[(int)(s < last) * s];
}

extern LOGGER logger_open_file(const char* file_name) {
    if (!file_name)
        return NULL;
    LOGGER l = (LOGGER)malloc(sizeof(struct logger));
    l->fd = fopen(file_name, "a");
    return l;
}

extern LOGGER logger_open_stdout() {
    LOGGER l = (LOGGER)malloc(sizeof(struct logger));
    l->fd = fdopen(STDOUT_FILENO, "w");
    return l;
}

extern void logger_close(LOGGER* l) {
    fclose((*l)->fd);
    (*l)->fd = NULL;
    free(*l);
    *l = NULL;
}
