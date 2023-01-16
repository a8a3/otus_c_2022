#define _POSIX_C_SOURCE 200809L
#include <config.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>
#include <yaml.h>

#define FILE_NAME_TOKEN "file_name"
#define SOCK_NAME_TOKEN "sock_name"
#define SOCK_NAME_LEN 64

typedef struct {
    char file_name[PATH_MAX + 1];
    char sock_name[SOCK_NAME_LEN];
} fszd_cfg_s;

fszd_cfg_s cfg;

void read_yaml(FILE* fd) {
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        syslog(LOG_ERR, "unable to initialize yaml parser");
        return;
    }
    yaml_parser_set_input_file(&parser, fd);
    yaml_token_t token;
    int token_type = 0;
    bool read_file_name = false;
    bool read_sock_name = false;
    do {
        yaml_parser_scan(&parser, &token);
        token_type = token.type;
        switch (token_type) {
            case YAML_SCALAR_TOKEN: {
                if (read_file_name) {
                    strncpy(cfg.file_name, (char*)token.data.scalar.value, PATH_MAX);
                    read_file_name = false;
                } else if (read_sock_name) {
                    strncpy(cfg.sock_name, (char*)token.data.scalar.value, SOCK_NAME_LEN);
                }
                read_file_name = (0 == strcmp((char*)token.data.scalar.value, FILE_NAME_TOKEN));
                read_sock_name = (0 == strcmp((char*)token.data.scalar.value, SOCK_NAME_TOKEN));
                break;
            }
            default:; // do nothing
        }
        yaml_token_delete(&token);
    } while (token_type != YAML_STREAM_END_TOKEN);

    yaml_parser_delete(&parser);
}

void read_cfg(const char* cfg_file) {
    FILE* fd = fopen(cfg_file, "r");

    if (!fd) {
        syslog(LOG_ERR, "unable to open file [%s]", cfg_file);
        return;
    }
    read_yaml(fd);
    fclose(fd);
    syslog(LOG_INFO, "watching for %s", cfg.file_name);
}

const char* cfg_get_file_name(void) { return cfg.file_name; }
const char* cfg_get_sock_name(void) { return cfg.sock_name; }
