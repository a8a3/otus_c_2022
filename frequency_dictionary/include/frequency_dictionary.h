#pragma once

#include "hash_map.h"
#include <string.h>

#define DICTIONARY_INITIAL_SZ 1024
#define WORD_DELIMETERS " \n"

typedef struct {
    hash_map* words_map;
} frequency_dictionary;

void frequency_dictionary_add_word(frequency_dictionary* fd, const char* word, size_t word_sz) {
    node* n = hash_map_insert(fd->words_map, word, word_sz);
    ++n->val;
}

frequency_dictionary* create_frequency_dictionary(FILE* fp) {
    frequency_dictionary* fd = (frequency_dictionary*)malloc(sizeof(frequency_dictionary));
    fd->words_map = create_hash_map(DICTIONARY_INITIAL_SZ);

    const int buf_sz = 4096;
    char buf[buf_sz];

    while (NULL != fgets(buf, buf_sz, fp)) {
        char* token = strtok(buf, WORD_DELIMETERS);
        while (token) {
            frequency_dictionary_add_word(fd, token, strlen(token));
            token = strtok(NULL, WORD_DELIMETERS);
        }
    }
    return fd;
}

void remove_frequency_dictionary(frequency_dictionary** fd) {
    remove_hash_map(&(*fd)->words_map);
    free(*fd);
    *fd = NULL;
}

void frequency_dictionary_print(frequency_dictionary* fd) {
    node* n = NULL;
    hash_map* map = fd->words_map;
    for (size_t i = 0; i < map->capacity; ++i) {
        n = map->nodes + i;
        if (NULL != n->key) {
            printf("word: %s, count: %d\n", n->key, n->val);
        }
    }
}
