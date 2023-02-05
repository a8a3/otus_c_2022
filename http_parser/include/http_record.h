#pragma once

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define URL_DEF_SZ 256
#define REF_DEF_SZ 256

typedef struct {
    char* url;
    size_t url_sz;
    char* referer;
    size_t referer_sz;
    size_t bytes;
} http_record;

void http_record_remove(http_record** record) {
    if (NULL == record || NULL == *record)
        return;
    free((*record)->url);
    free((*record)->referer);
    free(*record);
    *record = NULL;
}

http_record* http_rec_create(void) {
    http_record* rec = malloc(sizeof(http_record));
    assert(rec);

    rec->url = malloc(URL_DEF_SZ);
    assert(rec->url);
    memset(rec->url, 0, URL_DEF_SZ);
    rec->url_sz = URL_DEF_SZ - 1;

    rec->referer = malloc(REF_DEF_SZ);
    assert(rec->referer);
    memset(rec->referer, 0, REF_DEF_SZ);
    rec->referer_sz = REF_DEF_SZ - 1;

    rec->bytes = 0;
    return rec;
}

// return: 0 - happy camper
//       : 1 - error
int http_record_get(char* line, http_record** out_rec) {
    if (NULL == *out_rec) {
        *out_rec = http_rec_create();
        assert(*out_rec);
    }
    http_record* rec = *out_rec;
#define CHECK_TOKEN(chr, str)                                                                                          \
    do {                                                                                                               \
        if (NULL == (chr)) {                                                                                           \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)
    char* url_begin = strchr(line, '"');
    CHECK_TOKEN(url_begin, line);

    char* url_val_begin = strchr(url_begin, '/');
    CHECK_TOKEN(url_val_begin, line);

    char* url_val_end = strchr(url_val_begin, ' ');
    CHECK_TOKEN(url_val_end, line);

    size_t url_val_sz = url_val_end - url_val_begin;

    if (url_val_sz > rec->url_sz) {
        rec->url = realloc(rec->url, url_val_sz + 1);
        assert(rec->url);
    }
    strncpy(rec->url, url_val_begin, url_val_sz);
    rec->url[url_val_sz] = '\0';
    rec->url_sz = url_val_sz;

    char* status_begin = strchr(url_val_end + 1, ' ');
    CHECK_TOKEN(status_begin, line);

    char* end_ptr;
    char* size_begin = strchr(status_begin + 1, ' ');
    CHECK_TOKEN(size_begin, line);

    rec->bytes = strtoul(size_begin + 1, &end_ptr, 10);

    char* referer_begin = strchr(size_begin + 1, '"');
    CHECK_TOKEN(referer_begin, line);

    ++referer_begin;

    char* referer_end = strchr(referer_begin, '"');
    CHECK_TOKEN(referer_end, line);

    size_t referer_val_sz = referer_end - referer_begin;
    if (0 == strncmp(referer_begin, "-", referer_val_sz)) {
        return 0;
    }

    if (referer_val_sz > rec->referer_sz) {
        rec->referer = realloc(rec->referer, referer_val_sz + 1);
        assert(rec->referer);
    }

    strncpy(rec->referer, referer_begin, referer_val_sz);
    rec->referer[referer_val_sz] = '\0';
    rec->referer_sz = referer_val_sz;
    *out_rec = rec;
    return 0;
}
