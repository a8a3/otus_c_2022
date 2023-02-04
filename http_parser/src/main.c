#include <linux/limits.h>
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <dirent.h>
#include <getopt.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include <hash_map.h>
#include <http_record.h>

#define FILES_LIST_INIT_SZ 8
#define HASH_MAP_INIT_SZ 1024
#define URLS_STAT_SZ 10
#define REFS_STAT_SZ 10

#define MIN(a, b) (a) < (b) ? (a) : (b)

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

// C standard doesn't guarantee that all-bits-zero is the NULL pointer constant,
// explicitly nullify each pointer instead zeroing by memset
#define NULLIFY_PTRS(ptrs_arr, ptrs_arr_sz)                                                                            \
    do {                                                                                                               \
        for (size_t i = 0; i < (ptrs_arr_sz); ++i)                                                                     \
            (ptrs_arr)[i] = NULL;                                                                                      \
    } while (false)

typedef char** _Atomic atomic_iter;

char** files_list_create(char* dir_name, size_t* files_count) {
    DIR* dp = opendir(dir_name);
    if (NULL == dp) {
        perror(dir_name);
        exit(EXIT_FAILURE);
    }
    size_t files_list_sz = FILES_LIST_INIT_SZ;
    char** res = malloc((files_list_sz + 1) * sizeof(char*));
    assert(res);
    NULLIFY_PTRS(res, files_list_sz + 1);
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
                res = realloc(res, (files_list_sz + 1) * sizeof(char*));
                assert(res);
                NULLIFY_PTRS(res + f_idx, files_list_sz + 1 - f_idx);
            }
        }
    }
    closedir(dp);
    res[f_idx] = NULL;
    *files_count = f_idx;
    return res;
}

void files_list_remove(atomic_iter files_list) {
    char** iter = files_list;
    char* file;
    while ((file = *(iter++))) {
        free(file);
    }
    free(files_list);
    files_list = NULL;
}

typedef struct {
    hash_map* url;
    hash_map* referer;
    atomic_iter* files_iter;
} worker_param;

worker_param* worker_param_create(atomic_iter* files_iter) {
    worker_param* wp = (worker_param*)malloc(sizeof(worker_param));
    wp->url = create_hash_map(HASH_MAP_INIT_SZ);
    wp->referer = create_hash_map(HASH_MAP_INIT_SZ);
    wp->files_iter = files_iter;
    return wp;
}

void worker_param_remove(worker_param** wp) {
    remove_hash_map(&(*wp)->url);
    remove_hash_map(&(*wp)->referer);
    (*wp)->files_iter = NULL;
    free(*wp);
    wp = NULL;
}

void file_data_read(char* file_name, hash_map* url, hash_map* referer) {
    FILE* fd = fopen(file_name, "r");
    if (NULL == fd) {
        perror(file_name);
        exit(EXIT_FAILURE);
    }

    ssize_t read;
    size_t len;
    char* line = NULL;
    http_record* record = NULL;
    while (-1 != (read = getline(&line, &len, fd))) {
        if (1 == http_record_get(line, &record) || NULL == record)
            continue;

        if (record->url_sz > 0) {
            node* url_n = hash_map_insert(url, record->url, record->url_sz);
            assert(url_n);
            url_n->val += record->bytes;
        }

        if (record->referer_sz > 0) {
            node* ref_n = hash_map_insert(referer, record->referer, record->referer_sz);
            assert(ref_n);
            ref_n->val += 1;
        }
    }
    http_record_remove(&record);
    free(line);
    fclose(fd);
}

int worker_func(void* data) {
    worker_param* wp = (worker_param*)data;
    char* file;
    // it doesn't work-> *atomic_fetch_add_explicit(wp->files_iter, 1, memory_order_acq_rel)
    while ((file = *(*wp->files_iter)++)) {
        printf("%s file is being processed by %lu thread\n", file, pthread_self());
        file_data_read(file, wp->url, wp->referer);
    }
    thrd_exit(0);
}

void hash_map_add(hash_map* dst, hash_map* src) {
    node* n = NULL;
    for (size_t i = 0; i < src->capacity; ++i) {
        n = src->nodes + i;

        if (NULL != n->key) {
            node* added = hash_map_insert(dst, n->key, n->key_len);
            assert(NULL != added);
            added->val += n->val;
        }
    }
}

int nodes_desc_cmp(const void* p1, const void* p2) {
    node* n1 = *(node**)p1;
    node* n2 = *(node**)p2;

    if (n1->val < n2->val)
        return 1;
    if (n1->val > n2->val)
        return -1;

    return 0;
}

typedef node** nodes_arr;
void create_nodes_sorted_arr(hash_map* m, nodes_arr* arr, size_t* arr_sz) {
    node** node_ptr_arr = malloc(m->size * sizeof(node*));
    size_t arr_idx = 0;
    for (size_t i = 0; i < m->capacity; ++i) {
        node* n = m->nodes + i;
        if (NULL != n->key) {
            node_ptr_arr[arr_idx++] = n;
        }
    }
    qsort(node_ptr_arr, m->size, sizeof(node*), nodes_desc_cmp);
    *arr = node_ptr_arr;
    *arr_sz = m->size;
}

void get_stat(char* dir_name, int num_threads) {
    size_t files_count = 0;
    char** files_list = files_list_create(dir_name, &files_count);
    if (NULL == files_list[0]) {
        perror("directory is empty\n");
        exit(EXIT_FAILURE);
    }
    atomic_iter files_iter = files_list;
    num_threads = MIN((size_t)num_threads, files_count);

    worker_param** params = malloc(sizeof(worker_param*) * num_threads);
    assert(params);
    NULLIFY_PTRS(params, (size_t)num_threads);

    pthread_t* workers = malloc(sizeof(pthread_t) * num_threads);
    assert(workers);
    memset(workers, 0, sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; ++i) {
        params[i] = worker_param_create(&files_iter);
        thrd_create(workers + i, worker_func, params[i]);
    }

    thrd_join(workers[0], NULL);
    hash_map* url_result = params[0]->url;
    hash_map* ref_result = params[0]->referer;

    for (int i = 1; i < num_threads; ++i) {
        thrd_join(workers[i], NULL);
        hash_map_add(url_result, params[i]->url);
        hash_map_add(ref_result, params[i]->referer);
    }
    printf("\nstatistics calculation...\n");
    // hash_maps -> sorted arrays
    nodes_arr arr;
    size_t sz;
    create_nodes_sorted_arr(url_result, &arr, &sz);
    sz = MIN(sz, URLS_STAT_SZ);

    printf("\nMost trafic URLs:\n");
    for (size_t i = 0; i < sz; ++i) {
        printf("%lu) BYTES: [%d], URL: [%s]\n", i, arr[i]->val, arr[i]->key);
    }
    free(arr);

    create_nodes_sorted_arr(ref_result, &arr, &sz);
    sz = MIN(sz, REFS_STAT_SZ);

    printf("\nMost used REFERERs:\n");
    for (size_t i = 0; i < sz; ++i) {
        printf("%lu) TIMES: [%d], REFERER: [%s]\n", i, arr[i]->val, arr[i]->key);
    }
    free(arr);

    for (int i = 0; i < num_threads; ++i) {
        worker_param_remove(&params[i]);
    }
    free(params);
    free(workers);
    files_list_remove(files_list);
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
                    fprintf(stderr, "incorrect threads number passed: %s", optarg);
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
    get_stat(dir_name, num_threads);
    exit(EXIT_SUCCESS);
}
