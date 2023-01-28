#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hash_map.h>

#define FILES_LIST_INIT_SZ 8

void print_help(void) {
    printf("NAME:\n"
           "    http_parser\n"
           "        Parses http server logs in the specified directory\n"
           "OPTIONS:\n"
           "    -d, --directory\n"
           "        Directory with logs(current directory by default)\n"
           "    -t, --num_threads"
           "        Number of threads used(1 by default)"
           "    -h, --help\n"
           "        Print help\n");
}

char** create_files_list(char* dir_name) {
    DIR* dp = opendir(dir_name);
    if (NULL == dp) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    size_t files_list_sz = FILES_LIST_INIT_SZ;
    char** res = (char**)malloc((files_list_sz + 1) * sizeof(char*));
    size_t f_idx = 0;
    size_t entry_pos;
    struct dirent* entry;
    size_t file_name_len;
    size_t dir_name_len = strlen(dir_name);
    size_t entry_name_len;
    while (NULL != (entry = readdir(dp))) {
        if (DT_REG == entry->d_type) {
            entry_name_len = strlen(entry->d_name);
            file_name_len = dir_name_len + 1 + entry_name_len + 1;
            res[f_idx] = malloc(file_name_len);
            assert(res[f_idx]);

            entry_pos = 0;
            strcpy(res[f_idx] + entry_pos, dir_name);
            entry_pos += dir_name_len;

            strcpy(res[f_idx] + entry_pos, "/");
            entry_pos += 1;

            strcpy(res[f_idx] + entry_pos, entry->d_name);
            entry_pos += entry_name_len;
            res[f_idx][entry_pos] = '\0';

            ++f_idx;

            if (f_idx >= files_list_sz) {
                files_list_sz *= 2;
                res = (char**)realloc(res, (files_list_sz + 1) * sizeof(char*));
                assert(res);
            }
        }
    }
    closedir(dp);
    res[f_idx] = NULL;
    return res;
}

void remove_files_list(char** files_list) {
    char** files_iter = files_list;
    char* file;
    while ((file = *(files_iter++))) {
        free(file);
    }
    free(files_list);
    files_list = NULL;
}

void parse_line(char* line) {
    char* url_begin = strchr(line, '"');
    char* url_val_begin = strchr(url_begin, '/');
    printf("%s\n", url_val_begin);
    char* url_val_end = strchr(url_val_begin, ' ');
    size_t url_val_sz = url_val_end - url_val_begin;

    char* url_val = (char*)malloc(url_val_sz + 1);
    strncpy(url_val, url_val_begin, url_val_sz);
    url_val[url_val_sz] = '\0';
    printf("url    : [%s]\n", url_val);

    char* status_begin = strchr(url_val_end + 1, ' ');
    char* size_begin = strchr(status_begin + 1, ' ');
    char* end_ptr;
    long size = strtol(size_begin + 1, &end_ptr, 10);
    printf("size   : [%ld]\n", size);

    char* referer_begin = strchr(size_begin + 1, '"');
    ++referer_begin;
    char* referer_end = strchr(referer_begin + 1, '"');
    size_t referer_val_sz = referer_end - referer_begin;

    char* referer_val = (char*)malloc(referer_val_sz + 1);
    strncpy(referer_val, referer_begin, referer_val_sz);
    referer_val[referer_val_sz] = '\0';
    printf("referer: [%s]\n", referer_val);
    free(url_val);
    free(referer_val);
}

void read_file_data(char* file_name) {
    FILE* fd = fopen(file_name, "r");
    if (NULL == fd) {
        perror(file_name);
        exit(EXIT_FAILURE);
    }

    ssize_t read;
    size_t len;
    char* line = NULL;
    while (-1 != (read = getline(&line, &len, fd))) {
        // parse_line(line);
    }
    char* test_line = "52.28.236.88 - - [22/Jul/2020:23:33:53 +0000] \"GET "
                      "/.well-known/acme-challenge/H0bmB07yr_tHL1xbs_NTouoDBIQnRXm405Kbjx61hjg HTTP/1.1\" 200 87 "
                      "\"http://baneks.site/.well-known/acme-challenge/"
                      "H0bmB07yr_tHL1xbs_NTouoDBIQnRXm405Kbjx61hjg\" \"Mozilla/5.0 (compatible; Let's Encrypt "
                      "validation server; +https://www.letsencrypt.org)"
                      "-";
    parse_line(test_line);

    free(line);
    fclose(fd);
}

void parse(char* dir_name, int num_threads) {
    char** files_list = create_files_list(dir_name);
    if (NULL == files_list[0]) {
        perror("directory is empty\n");
        exit(EXIT_FAILURE);
    }
    char** files_iter = files_list;
    char* file;
    while ((file = *(files_iter++))) {
        read_file_data(file);
        ++file;
    }
    remove_files_list(files_list);
}

int main(int argc, char** argv) {
    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"directory", required_argument, 0, 'd'},
        {"num_threads", required_argument, 0, 't'},
        {0, 0, 0, 0},
    };
    char* dir_name = ".";
    int num_threads = 1;
    int opt_char = 0;

    while (-1 != (opt_char = getopt_long(argc, argv, ":d:t:h", opts, NULL))) {
        switch (opt_char) {
            case 'd': {
                dir_name = optarg;
                break;
            }
            case 't': {
                num_threads = atoi(optarg);
                if (0 == num_threads) {
                    fprintf(stderr, "unexpected threads number passed: %s", optarg);
                    exit(EXIT_FAILURE);
                }
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
    parse(dir_name, num_threads);
    exit(EXIT_SUCCESS);
}
