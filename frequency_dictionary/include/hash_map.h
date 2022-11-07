#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOLS_COUNT 256
typedef uint32_t hash_val;

// randomly placed 0-255 values
uint8_t symbols[SYMBOLS_COUNT] = {
    0x49, 0x0a, 0x6c, 0xb4, 0x38, 0xac, 0x59, 0x2c, 0x47, 0xb9, 0xd4, 0x3d, 0x2b, 0x14, 0x33, 0x35, 0x1e, 0x4b, 0x4c,
    0x80, 0x8a, 0xb7, 0xd6, 0x15, 0xde, 0xe9, 0x6b, 0xf6, 0x0b, 0xec, 0x74, 0x06, 0x42, 0x4d, 0x63, 0x7d, 0xad, 0x65,
    0x50, 0xb8, 0xe6, 0x8e, 0x54, 0x8c, 0x03, 0x04, 0x27, 0x31, 0x46, 0xa0, 0x71, 0x75, 0xa5, 0xd5, 0x3f, 0x28, 0xd9,
    0xe3, 0x0c, 0xf5, 0x69, 0x29, 0x89, 0xba, 0x3e, 0xb2, 0xc6, 0x0f, 0xb6, 0x16, 0xfb, 0xfc, 0xfe, 0x13, 0x2a, 0x39,
    0x61, 0x6e, 0x78, 0x86, 0x92, 0xa2, 0x10, 0x34, 0x1d, 0xa9, 0x7b, 0x5b, 0xaf, 0xbb, 0xc9, 0x62, 0xc2, 0x1c, 0x36,
    0x09, 0x6d, 0x88, 0x91, 0xc3, 0xdc, 0xf1, 0xf4, 0xab, 0xd0, 0x82, 0xdd, 0x0d, 0x66, 0xfd, 0x57, 0x18, 0x45, 0xc1,
    0xaa, 0x37, 0xda, 0x70, 0xe1, 0xe2, 0x56, 0x81, 0x53, 0xa4, 0xb3, 0x19, 0x85, 0x8d, 0xb5, 0x4a, 0x99, 0x25, 0xbf,
    0xc8, 0x87, 0xca, 0x58, 0xd1, 0x21, 0x26, 0x3a, 0xd7, 0x4e, 0x76, 0xdb, 0xcc, 0xa3, 0xed, 0xee, 0xef, 0xdf, 0x7f,
    0xf3, 0x20, 0x40, 0x44, 0x7c, 0x9c, 0xae, 0x68, 0xb0, 0xa1, 0x07, 0x0e, 0x5a, 0x51, 0x9d, 0xbc, 0xc5, 0xcf, 0x24,
    0x67, 0x77, 0x97, 0xa7, 0xbe, 0xd2, 0xd3, 0xe4, 0xe7, 0x98, 0x2d, 0xe8, 0x3b, 0x05, 0x3c, 0x41, 0x43, 0x48, 0x79,
    0x83, 0x90, 0x9b, 0x9e, 0x6a, 0xbd, 0xf2, 0xd8, 0x2e, 0x1b, 0x1a, 0x00, 0x52, 0x64, 0x73, 0x7a, 0x7e, 0x9f, 0x23,
    0xb1, 0x30, 0x12, 0x5e, 0x22, 0xc0, 0x2f, 0x17, 0x5d, 0xc4, 0x11, 0x55, 0xcd, 0xce, 0x72, 0xea, 0xe5, 0xeb, 0xf0,
    0xa8, 0x1f, 0x60, 0x84, 0x94, 0x96, 0xcb, 0x9a, 0xe0, 0x32, 0x5c, 0x8b, 0xf7, 0x02, 0xf8, 0xf9, 0xfa, 0x8f, 0x08,
    0x6f, 0x01, 0x93, 0x95, 0x4f, 0x5f, 0xa6, 0xff, 0xc7};

// it should be able to generate 2^32 unique hash values by Pirson's algorithm
hash_val pirsons_hash(const char* key, size_t key_len) {
    assert(key);
    assert(key_len > 0);

    hash_val hash_chunk, full_hash = 0;
    for (size_t i = 0; i < sizeof full_hash; ++i) {
        hash_chunk = symbols[(key[0] + i) % SYMBOLS_COUNT];

        for (size_t key_idx = 0; key_idx < key_len; ++key_idx) {
            hash_chunk = symbols[hash_chunk ^ key[key_idx]];
        }
        full_hash |= hash_chunk << i * 8;
    }
    return full_hash;
}

// implements double-hashing probing algorithm
hash_val pirsons_hash_val(const char* key, size_t key_len, size_t count, size_t attempt_idx) {
    const hash_val hash = pirsons_hash(key, key_len);
    return (hash % count + attempt_idx * (hash % (count - 1) + 1)) % count;
}

bool are_strings_equal(const char* key1, size_t key1_len, const char* key2, size_t key2_len) {
    return key1_len == key2_len && 0 == strncmp(key1, key2, key1_len);
}

// open addressing hash-map naive implementation
// hash map-node. i.e string - int pair
typedef struct {
    char* key;
    size_t key_len;
    int val;

} node;

// hash-map
typedef struct {
    node* nodes;
    size_t capacity;
    size_t size;

    hash_val (*hash_func)(const char*, size_t, size_t, size_t);
    bool (*keys_equality_func)(const char*, size_t, const char*, size_t);
} hash_map;

// creates a new hash-map object with desired capacity
hash_map* create_hash_map(size_t capacity) {
    hash_map* m = (hash_map*)malloc(sizeof(hash_map));
    if (NULL == m) {
        perror("unable to allocate hash-map");
        return NULL;
    }
    memset(m, 0, sizeof(hash_map));
    m->nodes = (node*)calloc(capacity, sizeof(node));

    if (NULL == m->nodes) {
        perror("unable to allocate hash-map storage");
        free(m);
        return NULL;
    }
    m->capacity = capacity;
    m->hash_func = &pirsons_hash_val;
    m->keys_equality_func = &are_strings_equal;
    return m;
}

// removes hash-map, resets pointer to the hash-map
void remove_hash_map(hash_map** map) {
    for (size_t i = 0; i < (*map)->capacity; ++i) {
        node* n = (*map)->nodes + i;
        free(n->key);
    }
    free((*map)->nodes);
    free(*map);
    *map = NULL;
}

// returns a node by a key if exists, NULL otherwise
node* hash_map_get_node(hash_map* m, const char* key, size_t key_len) {
    size_t attempt_idx = 0;
    node* n = NULL;
    do {
        n = m->nodes + m->hash_func(key, key_len, m->capacity - 1, attempt_idx);
        assert(n);
        ++attempt_idx;
    } while (NULL != n->key && !m->keys_equality_func(n->key, n->key_len, key, key_len));
    return NULL == n->key ? NULL : n;
}

void hash_map_rehash(hash_map*, size_t);

// creates a new node or assigned a node value if key exists in the hash-map,
// returns a pointer to inserted node or a pointer to existing node
node* hash_map_insert(hash_map* m, const char* key, size_t key_len) {
    size_t attempt_idx = 0;
    node *n = NULL, *candidate = NULL;
    do {
        candidate = m->nodes + m->hash_func(key, key_len, m->capacity - 1, attempt_idx);
        assert(candidate);
        if (NULL == candidate->key) { // node is free to use
            if (m->size < (m->capacity >> 1)) {
                char* new_key = (char*)malloc(key_len + 1);

                if (NULL == new_key) {
                    perror("unable to allocate hash-map node");
                    return NULL;
                }
                strncpy(new_key, key, key_len);
                new_key[key_len] = '\0';

                candidate->key = new_key;
                candidate->key_len = key_len;
                ++m->size;
            } else {
                hash_map_rehash(m, m->capacity << 1);
                candidate = hash_map_insert(m, key, key_len);
            }
            n = candidate;
        } else if (m->keys_equality_func(candidate->key, candidate->key_len, key, key_len)) {
            // keys matching, return node
            n = candidate;
        } else {
            // collision, new attempt to find a node
            ++attempt_idx;

            if (attempt_idx > m->capacity) {
                perror("unable to get a free hash-map node");
                return NULL;
            }
        }
    } while (NULL == n);
    return n;
}

void swap(size_t* s1, size_t* s2) {
    size_t tmp = *s1;
    *s1 = *s2;
    *s2 = tmp;
}

void hash_map_swap_data(hash_map* m1, hash_map* m2) {
    node* tmp = m1->nodes;
    m1->nodes = m2->nodes;
    m2->nodes = tmp;
    swap(&m1->capacity, &m2->capacity);
    swap(&m1->size, &m2->size);
}

void hash_map_rehash(hash_map* m, size_t new_capacity) {
    assert(new_capacity > m->capacity);

    hash_map* new_map = create_hash_map(new_capacity);
    node* n = NULL;
    for (size_t i = 0; i < m->capacity; ++i) {
        n = m->nodes + i;
        if (NULL != n->key) {
            node* inserted = hash_map_insert(new_map, n->key, n->key_len);
            assert(NULL != inserted);
            inserted->val = n->val;
        }
    }
    hash_map_swap_data(m, new_map);
    remove_hash_map(&new_map);
}
