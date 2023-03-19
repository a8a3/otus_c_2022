#ifndef _WTF_TABLE_H_
#define _WTF_TABLE_H_

#include <assert.h>
#include <bits/stdint-uintn.h>
#include <limits.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITS_COUNT(entity) sizeof((entity)) * (CHAR_BIT)

typedef struct {
    size_t hash_;
    const void* value_;
} node_t;
typedef node_t* node_ptr_t;
typedef _Atomic(node_t*) atomic_node_ptr_t; // i.e atomic pointer to non-atomic node_t

// TODO: do I need atomics in is_... funcs?
// node is array if 2nd LSB is turned on

// mark LSB
void mark_data_node(atomic_uintptr_t* n) { atomic_fetch_or_explicit(n, (uintptr_t)1U, memory_order_release); }

// return value with unmarked LSB
atomic_uintptr_t unmark_data_node(atomic_uintptr_t n) {
    // TODO: do I need atomic operation here?
    atomic_fetch_and_explicit(&n, ~((uintptr_t)1U), memory_order_release);
    return n;
}

// node is marked if LSB is turned on
bool is_marked_node(atomic_uintptr_t n) { return n & 1U; }
bool is_array_node(atomic_uintptr_t n) { return n & (1U << 1); }

typedef union {
    atomic_node_ptr_t as_ptr;
    atomic_uintptr_t as_uint;
} atomic_ptr_uint_u;

typedef union {
    node_ptr_t as_ptr;
    uintptr_t as_uint;
} ptr_uint_u;

// wtf stands for WaiT Free
typedef struct {
    atomic_ptr_uint_u* head_;
    size_t head_sz_; // head and all array nodes have same size
    size_t max_fail_count;
} wtf_table_t;

// Create table instance and return pointer on it
wtf_table_t* wtf_table_create(size_t head_sz) {
    wtf_table_t* t = malloc(sizeof(wtf_table_t));
    t->head_ = malloc(sizeof(atomic_ptr_uint_u) * head_sz);
    t->head_sz_ = head_sz;
    memset(t->head_, 0, sizeof(atomic_ptr_uint_u) * head_sz);
    // TODO: magic number
    t->max_fail_count = 10;
    return t;
}

node_t* allocate_node(size_t hash, const void* value) {
    node_t* n = malloc(sizeof(node_t));
    n->hash_ = hash;
    n->value_ = value;
    return n;
}

// Insert key-value pair it table.
// here key is precalculated hash of a real key
// If the table holds the key, value overwrites the previous value, and wtf_table_insert returns the previous value.
// Otherwise, key value pair is added to the table, and wtf_table_insert returns NULL
void* wtf_table_insert(wtf_table_t* t, size_t hash, const void* value) {
    ptr_uint_u new_node;
    new_node.as_ptr = allocate_node(hash, value);
    // TODO: handle malloc error
    atomic_ptr_uint_u* local = t->head_; // start from head

    uint8_t bits_shift = (uint8_t)log2(BITS_COUNT(hash)), fail_count = 0;
    for (size_t r = 0; r < BITS_COUNT(hash); r += bits_shift) {
        // TODO: 10th r should be 4 bits length
        uint8_t pos = hash & (t->head_sz_ - 1); // position in local(current) array node
        printf("insert: pos in local: %u\n", pos);
        hash >>= bits_shift;
        fail_count = 0;

        for (;;) {
            if (fail_count > t->max_fail_count) {
                mark_data_node(&local[pos].as_uint);
            }

            atomic_ptr_uint_u node = local[pos];
            if (is_array_node(node.as_uint)) {
                local = &node;
                break; // for
            } else if (is_marked_node(node.as_uint)) {
                // TODO:   local = expand_table(local, pos, node, r);
                break;
            } else if (NULL == node.as_ptr) {
                // TODO: explicit cmp func
                if (atomic_compare_exchange_strong(&local[pos].as_uint, (uintptr_t*)&node.as_uint, new_node.as_uint)) {
                    return NULL; // node added
                } else {
                    // TBD
                }
            } else {
                // do I need atomic pointer here?
                if (((node_ptr_t)(node.as_ptr))->hash_ == new_node.as_ptr->hash_) {

                } else {
                    // TODO:   local = expand_table(local, pos, node, r);
                }
            }
        }
    }
    return NULL;
}

// Return value associated with the key, if the table doesn't hold the key, return NULL
const void* wtf_table_find(wtf_table_t* t, size_t hash) {
    atomic_ptr_uint_u* local = t->head_;
    size_t local_hash = hash;
    uint8_t bits_shift = (uint8_t)log2(BITS_COUNT(hash)), pos;
    for (size_t r = 0; r < BITS_COUNT(hash); r += bits_shift) {
        pos = local_hash & (t->head_sz_ - 1);
        printf("find: pos in local: %u\n", pos);
        local_hash >>= bits_shift;
        atomic_ptr_uint_u node;
        node.as_uint = unmark_data_node(local[pos].as_uint);
        if (is_array_node(node.as_uint)) {
            printf("find: array node in pos: %u\n", pos);
            local = &node;
        } else {
            printf("find: regular node in pos: %u\n", pos);
            if (node.as_ptr->hash_ == hash) {
                printf("find: regular node in pos: %u is found\n", pos);
                return node.as_ptr->value_;
            } else {
                printf("find: hash mismatched: requested: %lu, found: %lu\n", hash, node.as_ptr->hash_);
                return NULL;
            }
        }
    }
    return NULL;
}

// Deallocate table content and the table itself, set t to NULL
void wtf_table_destroy(wtf_table_t** t) {
    // TODO: deallocate content
    free(*t);
    *t = NULL;
}

#endif // _WTF_TABLE_H_
