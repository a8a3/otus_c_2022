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

#define BITS_COUNT(entity) sizeof(entity) * CHAR_BIT
typedef size_t hash_t;

typedef struct {
    size_t hash_;
    void* value_;
} node_t;
typedef void* node_ptr_t;
typedef _Atomic(void*) atomic_node_ptr_t; // i.e atomic pointer to non-atomic node_t

void unmark_atomic_node(atomic_node_ptr_t* n) {
    atomic_fetch_and_explicit((atomic_uintptr_t*)n, ~((uintptr_t)1U), memory_order_release);
}

void unmark_atomic_array(atomic_node_ptr_t* n) {
    atomic_fetch_and_explicit((atomic_uintptr_t*)n, ~((uintptr_t)1U << 1), memory_order_release);
}

bool is_node_marked(node_ptr_t* n) { return (uintptr_t)n & 1U; }

bool is_atomic_node_marked(atomic_node_ptr_t n) {
    return (uintptr_t)atomic_load_explicit(&n, memory_order_acquire) & 1U;
}

bool is_atomic_array(atomic_node_ptr_t a) {
    return (uintptr_t)atomic_load_explicit(&a, memory_order_acquire) & (1U << 1);
}

void mark_as_array(void** a) {
    *a = (uintptr_t*)((uintptr_t)*a | (uintptr_t)1U << 1); // NOLINT
}

void mark_node(void** n) {
    *n = (uintptr_t*)((uintptr_t)*n | (uintptr_t)1U); // NOLINT
}

void* get_unmarked_node(void* n) {
    return (void*)((uintptr_t)n & ~((uintptr_t)1U)); // NOLINT
}

void unmark_array(void** a) {
    *a = (uintptr_t*)((uintptr_t)*a & ~((uintptr_t)1U << 1)); // NOLINT
}

void* get_unmarked_array(void* a) {
    return (void*)((uintptr_t)a & ~((uintptr_t)1U << 1)); // NOLINT
}

void mark_atomic_node(atomic_node_ptr_t* n) {
    atomic_fetch_or_explicit((atomic_uintptr_t*)n, (uintptr_t)1U, memory_order_release);
}

void mark_atomic_array(atomic_node_ptr_t* n) {
    atomic_fetch_or_explicit((atomic_uintptr_t*)n, (uintptr_t)1U << 1, memory_order_release);
}

atomic_node_ptr_t* get_unmarked_atomic_node(atomic_node_ptr_t n) {
    return (atomic_node_ptr_t*)((uintptr_t)n & ~(uintptr_t)1U); // NOLINT
}

atomic_node_ptr_t* get_unmarked_atomic_array(atomic_node_ptr_t a) {
    return (atomic_node_ptr_t*)((uintptr_t)a & ~(uintptr_t)1U << 1); // NOLINT
}

bool is_array(void* a) { return (uintptr_t)a & ((uintptr_t)1U << 1); }

// wtf stands for WaiT Free
typedef struct {
    atomic_node_ptr_t* head_;
    size_t arr_sz_; // head and all array nodes(except last-level arrays) have same size
    size_t max_fail_count;
    uint8_t bits_shift;
} wtf_table_t;

// Create table instance and return pointer on it
wtf_table_t* wtf_table_create(size_t arr_sz) {
    wtf_table_t* t = malloc(sizeof(wtf_table_t));
    t->head_ = malloc(sizeof(atomic_node_ptr_t) * arr_sz);
    t->arr_sz_ = arr_sz;
    memset(t->head_, 0, sizeof(atomic_node_ptr_t) * arr_sz);
    // TODO: magic number
    t->max_fail_count = 10;
    t->bits_shift = (uint8_t)log2(BITS_COUNT(hash_t));
    return t;
}

node_t* allocate_node(size_t hash, void* value) {
    node_t* n = malloc(sizeof(node_t));
    if (NULL == n) {
        return NULL;
    }
    n->hash_ = hash;
    n->value_ = value;
    return n;
}

// TODO: check if I need this BITS_COUNT macro, when I do know size of array itself(t->arr_sz_)
size_t wtf_table_get_arr_sz(wtf_table_t* t, size_t lvl) {
    return lvl < BITS_COUNT(t->arr_sz_) / t->bits_shift ? t->arr_sz_ : (1L << (BITS_COUNT(t->arr_sz_) % t->bits_shift));
}

// replace node value with new array
// insert value into new array
// return new array to caller side
atomic_node_ptr_t* expand_table(wtf_table_t* t, atomic_node_ptr_t* expanded_node, size_t r) {
    node_ptr_t node = atomic_load_explicit(expanded_node, memory_order_acquire);
    uint8_t next_level = r / t->bits_shift + 1;
    // new array size is equal to t->arr_sz_ for 0-9 levels only
    size_t new_array_sz = wtf_table_get_arr_sz(t, next_level);
    printf("exp: new array sz: %lu, level: %u\n", new_array_sz, next_level);

    atomic_node_ptr_t* new_array = malloc(sizeof(atomic_node_ptr_t) * new_array_sz);
    if (NULL == new_array) {
        return NULL;
    }
    memset(new_array, 0, sizeof(atomic_node_ptr_t) * new_array_sz);
    printf("exp: new array allocated: %p\n", (void*)new_array);

    // calc expanded node idx in new array
    uint8_t expanded_node_new_pos = (((node_t*)node)->hash_ >> (t->bits_shift + r)) & (new_array_sz - 1);
    // insert expanded node in new array
    new_array[expanded_node_new_pos] = node;
    printf("exp: expanded node in new arr: %p, pos: %u\n", new_array[expanded_node_new_pos], expanded_node_new_pos);

    // insert expanded node in new array
    mark_as_array((void**)&new_array);

    printf("exp: try insert: obj: %p, exp: %p, desired: %p\n", *expanded_node, node, (void*)new_array);
    // replace node with array
    if (atomic_compare_exchange_strong(expanded_node, &node, new_array)) {
        printf("exp: new array inserted: %p\n", (void*)*expanded_node);
        return new_array;
    } else {

        printf("exp: unable to insert new array: %p\n", (void*)new_array);
        // other thread has inserted array in this node already
        if (is_array(&node)) {
            // the node was already expanded, free stuff created above
            free(new_array);
            return expanded_node;
        }
    }
    return NULL;
}

// Insert key-value pair it table.
// here key is precalculated hash of a real key
// If the table holds the key, value overwrites the previous value, and wtf_table_insert returns the previous value.
// Otherwise, key value pair is added to the table, and wtf_table_insert returns NULL
// TODO: check if all actions with local[pos] should be atomic...
void* wtf_table_insert(wtf_table_t* t, hash_t hash, void* value) {
    node_ptr_t new_node;
    new_node = allocate_node(hash, value);
    printf("ins: node allocated at %p\n", (void*)new_node);

    if (NULL == new_node) {
        return NULL;
    }
    atomic_node_ptr_t* cur_arr = t->head_; // start from head
    uint8_t fail_count = 0, pos = 0;

    for (size_t r = 0; r < BITS_COUNT(hash); r += t->bits_shift) {
        printf("ins: cur_arr: %p\n", cur_arr);
        // TODO: 10th r should be 4 bits length
        pos = hash & (t->arr_sz_ - 1); // position in local(current) array node
        node_ptr_t cur_node = cur_arr[pos];
        printf("ins: hash: %zu, pos in local: %u\n", hash, pos);
        hash >>= t->bits_shift;
        fail_count = 0;

        for (;;) {
            if (fail_count > t->max_fail_count) {
                mark_node(&cur_node);
            }

            printf("ins: candidate at %u pos is %p\n", pos, cur_node);
            if (is_atomic_array(cur_node)) {
                cur_arr = get_unmarked_atomic_array(cur_node);
                break; // for
            } else if (is_atomic_node_marked(cur_node)) {
                // TODO:   local = expand_table(local, pos, node, r);
                break;
            } else if (NULL == cur_node) {
                // TODO: explicit cmp func
                printf("ins: try insert: obj: %p, exp: %p, desired: %p\n", cur_arr[pos], cur_node, new_node);
                if (atomic_compare_exchange_strong(&cur_arr[pos], &cur_node, new_node)) {
                    printf("ins: %p is inserted into: %p, at %u, level: %lu\n", cur_arr[pos], cur_arr, pos,
                           r / t->bits_shift);
                    return NULL; // node added
                } else {
                    // TBD
                    // increase fail_count?
                }
            } else {
                // TODO: do I need atomic pointer here?
                // TODO: is it concurrently safe to cmp in that way?
                if (((node_t*)cur_node)->hash_ == ((node_t*)new_node)->hash_) {
                    if (atomic_compare_exchange_strong((atomic_uintptr_t*)&cur_arr[pos], (uintptr_t*)&cur_node,
                                                       (uintptr_t)new_node)) {
                        printf("ins: %p is inserted at %u, level: %lu\n", (void*)new_node, pos, r / t->bits_shift);
                        return cur_node; // prev node
                    } else {
                        // TBD
                    }
                } else {
                    printf("ins: expand %p\n", (void*)cur_node);
                    atomic_node_ptr_t* expanded = expand_table(t, &cur_arr[pos], r);
                    printf("ins: expanded %p\n", (void*)expanded);

                    if (!is_array(expanded)) {
                        ++fail_count;
                    } else {
                        cur_arr = get_unmarked_atomic_array(expanded);
                        printf("ins: set cur_arr to %p\n", (void*)cur_arr);
                        break; // inner for
                    }
                }
            }
        }
    }
    return NULL;
}

// Return value associated with the key, if the table doesn't hold the key, return NULL
void* wtf_table_find(wtf_table_t* t, size_t hash) {
    atomic_node_ptr_t* cur_arr = t->head_;
    atomic_node_ptr_t node;

    size_t cur_hash = hash;
    uint8_t pos;
    printf("fnd: ==> find %lu hash\n", hash);

    for (size_t r = 0; r < BITS_COUNT(hash); r += t->bits_shift) {
        uint8_t cur_level = r / t->bits_shift;

        pos = cur_hash & (t->arr_sz_ - 1);
        printf("fnd: level: %u, pos: %u\n", cur_level, pos);
        cur_hash >>= t->bits_shift;

        // TODO: return atomic ptr?
        node = get_unmarked_node(cur_arr[pos]);

        if (is_atomic_array(node)) {
            printf("fnd: array node %p in pos: %u, level: %lu\n", (void*)node, pos, r / t->bits_shift);
            unmark_atomic_array(&node);
            cur_arr = node;
            printf("fnd: unmarked array: %p\n", (void*)cur_arr);
        } else {
            printf("fnd: regular node %p in pos: %u, level: %lu\n", node, pos, r / t->bits_shift);
            if (NULL == node) {
                printf("fnd: node is empty\n");
                return NULL;
            } else if (((node_t*)(node))->hash_ == hash) {
                printf("fnd: hash matched: %lu is found\n", hash);
                return ((node_t*)(node))->value_;
            } else {
                printf("fnd: hash mismatched: requested: %lu, found: %lu\n", hash, ((node_t*)(node))->hash_);
                return NULL;
            }
        }
    }
    return NULL;
}

// TODO: should it have some atomic semanthics?
typedef void (*doer)(atomic_node_ptr_t, void*);
void wtf_table_for_each_helper(wtf_table_t* t, atomic_node_ptr_t* a, size_t lvl, doer f, void* user_data) {

    for (size_t i = 0; i < wtf_table_get_arr_sz(t, lvl); ++i) {
        atomic_node_ptr_t node = a[i];
        if (NULL == node) {
            continue;
        } else if (is_atomic_array(node)) {
            atomic_node_ptr_t unmarked_arr = get_unmarked_atomic_array(node);
            wtf_table_for_each_helper(t, unmarked_arr, lvl + 1, f, user_data);
            f(unmarked_arr, user_data);
        } else if (is_atomic_node_marked(node)) {
            f(get_unmarked_atomic_node(node), user_data);
        } else {
            f(node, user_data);
        }
    }
}

// TODO: take in account last level array size...
void wtf_table_for_each(wtf_table_t* t, doer f, void* user_data) {
    wtf_table_for_each_helper(t, t->head_, 0, f, user_data);
}

void destroy_node(atomic_node_ptr_t node, void* user_data) {
    (void)user_data;
    free(node);
}

// Deallocate table content and the table itself, set t to NULL
void wtf_table_destroy(wtf_table_t** t) {
    wtf_table_for_each(*t, destroy_node, NULL);
    free((*t)->head_);
    free(*t);
    *t = NULL;
}

#endif // _WTF_TABLE_H_
