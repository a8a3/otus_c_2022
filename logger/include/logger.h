#pragma once
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>

// simple logging library
typedef struct {
    FILE* fd;
} logger;
typedef logger* LOGGER;

extern LOGGER logger_open_file(const char* file_name);
extern LOGGER logger_open_stdout();
extern void logger_print(LOGGER, const char* format, ...);
extern void logger_close(LOGGER*);

typedef enum { info = 0, warning, error, debug } severity;

extern char* severity_as_str(severity s);
extern char* get_current_time_us();

#define LOG_IMPL(logger_instance, severity, format, ...)                                                               \
    logger_print(logger_instance, "[%s] %s:%04d [%s] " format "\n", get_current_time_us(), __FILE__, __LINE__,         \
                 severity_as_str(severity), ##__VA_ARGS__);

#define LOG_OPEN_FILE(file_name) logger_open_file(file_name)
#define LOG_OPEN_STDOUT() logger_open_stdout()
#define LOG_CLOSE(logger_instance) logger_close(logger_instance)

#define LOG_INFO(logger_instance, format, ...) LOG_IMPL(logger_instance, info, format, ##__VA_ARGS__)
#define LOG_WARN(logger_instance, format, ...) LOG_IMPL(logger_instance, warning, format, ##__VA_ARGS__)

#define BACKTRACE_BUF_SZ 32
#define LOG_ERR(logger_instance, format, ...)                                                                          \
    do {                                                                                                               \
        LOG_IMPL(logger_instance, error, format, ##__VA_ARGS__)                                                        \
        void* buf[BACKTRACE_BUF_SZ];                                                                                   \
        int num = backtrace(buf, sizeof buf);                                                                          \
        fseek(logger_instance->fd, 0, SEEK_END);                                                                       \
        backtrace_symbols_fd(buf, num, fileno(logger_instance->fd));                                                   \
        fflush(logger_instance->fd);                                                                                   \
    } while (0)

#ifdef NDEBUG
#define LOG_DEBUG(logger_instance, format, ...) ((void)0)
#else
#define LOG_DEBUG(logger_instance, format, ...) LOG_IMPL(logger_instance, debug, format, ##__VA_ARGS__)

#endif // NDEBUG
