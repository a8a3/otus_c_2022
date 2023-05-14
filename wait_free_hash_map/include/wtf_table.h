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

typedef struct {
    size_t hash_;
    void* value_;
} node_t;
typedef node_t* _Atomic atomic_node_ptr_t; // i.e atomic pointer to non-atomic node_t

void unmark_atomic_node(atomic_node_ptr_t* n) {
    atomic_fetch_and_explicit((atomic_uintptr_t*)n, ~((uintptr_t)1U), memory_order_release);
}

void unmark_atomic_array(atomic_node_ptr_t* n) {
    atomic_fetch_and_explicit((atomic_uintptr_t*)n, ~((uintptr_t)1U << 1), memory_order_release);
}

bool is_node_marked(node_t* n) { return (uintptr_t)n & 1U; }

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
} wtf_table_t;

#define ARR_SZ 64
#define BIT_SHIFT 6       // log2(ARR_SZ)
#define MAX_LVL_ARR_SZ 16 // 1 << (ARR_SZ % BIT_SHIFT)
#define MAX_LVL 10        // ARR_SZ / BIT_SHIFT
#define MAX_FAIL_CNT 8

#define CUR_ARR_SZ(lvl) ((lvl) < MAX_LVL) ? ARR_SZ : MAX_LVL_ARR_SZ

wtf_table_t* wtf_table_create(void) {
    wtf_table_t* t = malloc(sizeof(wtf_table_t));
    t->head_ = malloc(sizeof(atomic_node_ptr_t) * ARR_SZ);
    memset(t->head_, 0, sizeof(atomic_node_ptr_t) * ARR_SZ);
    return t;
}

node_t* wtf_table_allocate_node(size_t hash, void* value) {
    node_t* n = malloc(sizeof(node_t));
    if (NULL == n) {
        return NULL;
    }
    n->hash_ = hash;
    n->value_ = value;
    return n;
}

// replace node value with new array
// insert value into new array
// return new array to caller side
atomic_node_ptr_t* wtf_table_expand_table(atomic_node_ptr_t* expanded_node, size_t s) {
    node_t* node = atomic_load_explicit(expanded_node, memory_order_acquire);
    uint8_t next_level = s / BIT_SHIFT + 1;
    // new array size is equal to t->arr_sz_ for 0-9 levels only
    size_t new_array_sz = CUR_ARR_SZ(next_level);
    printf("exp: new array sz: %lu, level: %u\n", new_array_sz, next_level);

    atomic_node_ptr_t* new_array = malloc(sizeof(atomic_node_ptr_t) * new_array_sz);
    if (NULL == new_array) {
        return NULL;
    }
    memset(new_array, 0, sizeof(atomic_node_ptr_t) * new_array_sz);
    printf("exp: new array allocated: %p\n", (void*)new_array);

    // calc expanded node idx in new array
    uint8_t expanded_node_new_pos = node->hash_ >> (BIT_SHIFT + s) & (new_array_sz - 1);
    // insert expanded node in new array
    new_array[expanded_node_new_pos] = node;
    printf("exp: expanded node in new arr: %p, pos: %u\n", (void*)new_array[expanded_node_new_pos], expanded_node_new_pos);

    // insert expanded node in new array
    mark_as_array((void**)&new_array);

    printf("exp: try insert: obj: %p, exp: %p, desired: %p\n", (void*)*expanded_node, (void*)node, (void*)new_array);
    // replace node with array
    if (atomic_compare_exchange_strong(expanded_node, &node, (void*)new_array)) {
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

// Insert key-value pair in table.
// key is precalculated hash value of a real key
// If the table holds the key, value overwrites existing value
// Otherwise, key value pair is added to the table
bool wtf_table_insert(wtf_table_t* t, size_t hash, void* value) {
    node_t* new_node = wtf_table_allocate_node(hash, value);
    printf("ins: node allocated at %p\n", (void*)new_node);

    if (NULL == new_node) {
        // TODO: raise an error?
        return false;
    }
    atomic_node_ptr_t* cur_arr = t->head_;
    uint8_t fail_count = 0, pos = 0;

    for (size_t s = 0; s < ARR_SZ; s += BIT_SHIFT) {
        printf("ins: cur_arr: %p\n", (void*)cur_arr);
        pos = hash & (ARR_SZ - 1); // position in current array
        printf("ins: hash: %zu, pos in local: %u\n", hash, pos);

        node_t* cur_node = atomic_load(&cur_arr[pos]);
        hash >>= BIT_SHIFT;
        fail_count = 0;

        for (;;) {
            if (fail_count > MAX_FAIL_CNT) {
                // TODO: mark ptr in array(i.e cur_arr[pos]), not local val
                mark_node((void**)&cur_node);
            }

            printf("ins: candidate at %u pos is %p\n", pos, (void*)cur_node);
            if (is_array(cur_node)) {
                cur_arr = get_unmarked_array(cur_node);
                break; // for
            } else if (is_node_marked(cur_node)) {
                // TODO:   local = expand_table(local, pos, node, r);
                break;
            } else if (NULL == cur_node) {
                printf("ins: try insert: obj: %p, exp: %p, desired: %p\n", (void*)cur_arr[pos], (void*)cur_node, (void*)new_node);
                if (atomic_compare_exchange_strong(&cur_arr[pos], &cur_node, new_node)) {
                    printf("ins: %p is inserted into: %p, at %u, level: %lu\n", (void*)cur_arr[pos], (void*)cur_arr, pos,
                           s / BIT_SHIFT);
                    return true;
                } else {
                    // TBD
                    // increase fail_count?
                }
            } else {
                size_t cur_hash = cur_node->hash_;
                size_t new_hash = new_node->hash_;
                if (cur_hash == new_hash) {
                    if (atomic_compare_exchange_strong(&cur_arr[pos], &cur_node, new_node)) {
                        printf("ins: %p is inserted at %u, level: %lu\n", (void*)new_node, pos, s / BIT_SHIFT);
                        free(cur_node);
                        return true;
                    } else {
                        // TBD
                    }
                } else {
                    printf("ins: expand %p\n", (void*)cur_node);
                    atomic_node_ptr_t* expanded = wtf_table_expand_table(&cur_arr[pos], s);
                    printf("ins: expanded %p\n", (void*)expanded);

                    if (!is_atomic_array((void*)expanded)) {
                        ++fail_count;
                    } else {
                        cur_arr = get_unmarked_atomic_array((void*)expanded);
                        printf("ins: set cur_arr to %p\n", (void*)cur_arr);
                        break; // inner for
                    }
                }
            }
        }
    }
    return false;
}

// Return value associated with the key, if the table doesn't hold the key, return NULL
void* wtf_table_find(wtf_table_t* t, size_t hash) {
    atomic_node_ptr_t* cur_arr = t->head_;
    node_t* node;
    size_t cur_hash = hash;
    uint8_t pos;
    printf("fnd: ==> find %lu hash\n", hash);

    for (size_t r = 0; r < ARR_SZ; r += BIT_SHIFT) {
        pos = cur_hash & (ARR_SZ - 1);
        printf("fnd: level: %lu, pos: %u\n", r / BIT_SHIFT, pos);
        cur_hash >>= BIT_SHIFT;

        node = get_unmarked_node(atomic_load(&cur_arr[pos]));

        if (is_array(node)) {
            printf("fnd: array node %p in pos: %u, level: %lu\n", (void*)node, pos, r / BIT_SHIFT);
            unmark_array((void**)&node);
            cur_arr = (void*)node;
            printf("fnd: unmarked array: %p\n", (void*)cur_arr);
        } else {
            printf("fnd: regular node %p in pos: %u, level: %lu\n", (void*)node, pos, r / BIT_SHIFT);
            if (NULL == node) {
                printf("fnd: node is empty\n");
                return NULL;
            } else if (node->hash_ == hash) {
                printf("fnd: hash matched: %lu is found\n", hash);
                return node->value_;
            } else {
                printf("fnd: hash mismatched: requested: %lu, found: %lu\n", hash, node->hash_);
                return NULL;
            }
        }
    }
    return NULL;
}


typedef void (*doer)(atomic_node_ptr_t, void*);
void wtf_table_for_each_helper(wtf_table_t* t, atomic_node_ptr_t* node, size_t lvl, doer f, void* user_data) {
    size_t arr_sz = CUR_ARR_SZ(lvl);
    for (size_t i = 0; i < arr_sz; ++i) {
        node_t* cur_node = atomic_load(&node[i]);
        if (NULL == cur_node) {
            continue;
        } else if (is_array(cur_node)) {
            node_t* unmarked_arr = get_unmarked_array(cur_node);
            wtf_table_for_each_helper(t, (void*)unmarked_arr, lvl + 1, f, user_data);
            f(unmarked_arr, user_data);
        } else if (is_node_marked(cur_node)) {
            f(get_unmarked_node(cur_node), user_data);
        } else {
            f(cur_node, user_data);
        }
    }
}

void wtf_table_for_each(wtf_table_t* t, doer f, void* user_data) {
    wtf_table_for_each_helper(t, t->head_, 0, f, user_data);
}

void wtf_table_destroy_node(atomic_node_ptr_t node, void* user_data) {
    (void)user_data;
    free(node);
}

// Deallocate table content and the table itself, set t to NULL
void wtf_table_destroy(wtf_table_t** t) {
    wtf_table_for_each(*t, wtf_table_destroy_node, NULL);
    free((*t)->head_);
    free(*t);
    *t = NULL;
}

#endif // _WTF_TABLE_H_
