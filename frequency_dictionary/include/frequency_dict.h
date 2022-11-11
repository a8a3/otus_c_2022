#pragma once

#include <stdio.h>
#include <string.h>

#include "hash_map.h"

#define FILE_BUF_SIZЕ 4096
#define WORD_DELIMETERS " .,;:!?\n"

void hash_map_init(hash_map* hm, FILE* fd) {
    char buf[FILE_BUF_SIZЕ];
    while (NULL != fgets(buf, FILE_BUF_SIZЕ, fd)) {
        char* token = strtok(buf, WORD_DELIMETERS);
        while (token) {
            node* n = hash_map_insert(hm, token, strlen(token));
            if (NULL != n) {
                ++n->val;
            }
            token = strtok(NULL, WORD_DELIMETERS);
        }
    }
}
