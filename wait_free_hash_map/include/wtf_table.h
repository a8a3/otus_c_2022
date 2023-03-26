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
    const void* value_;
} node_t;
typedef node_t* node_ptr_t;
typedef _Atomic(node_t*) atomic_node_ptr_t; // i.e atomic pointer to non-atomic node_t

void mark_atomic_node(atomic_node_ptr_t* n) {
    atomic_fetch_or_explicit((atomic_uintptr_t*)n, (uintptr_t)1U, memory_order_release);
}

void unmark_atomic_node(atomic_node_ptr_t* n) {
    atomic_fetch_and_explicit((atomic_uintptr_t*)n, ~((uintptr_t)1U), memory_order_release);
}

node_ptr_t* get_unmarked_node(atomic_node_ptr_t* n) {
    return (node_ptr_t*)((uintptr_t)atomic_load_explicit(n, memory_order_acquire) & ~(uintptr_t)1U); // NOLINT
}

bool is_node_marked(node_ptr_t* n) { return (uintptr_t)n & 1U; }

bool is_atomic_node_marked(atomic_node_ptr_t* n) {
    return (uintptr_t)atomic_load_explicit(n, memory_order_acquire) & 1U;
}

bool is_atomic_array(atomic_node_ptr_t* a) {
    return (uintptr_t)atomic_load_explicit(a, memory_order_acquire) & (1U << 1);
}

void mark_as_array(void** a) {
    *a = (uintptr_t*)((uintptr_t)*a | (uintptr_t)1U << 1); // NOLINT
}

// unmark 2nd LSB
void unmark_array(void** a) {
    *a = (uintptr_t*)((uintptr_t)*a & ~((uintptr_t)1U << 1)); // NOLINT
}

// return value with unmarked 2nd LSB
void* get_unmarked_array(void* a) {
    return (void*)((uintptr_t)a & ~((uintptr_t)1U << 1)); // NOLINT
}

bool is_array(void* a) { return (uintptr_t)a & ((uintptr_t)1U << 1); }

// wtf stands for WaiT Free
typedef struct {
    atomic_node_ptr_t* head_;
    size_t arr_sz_; // head and all array nodes have same size
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

node_t* allocate_node(size_t hash, const void* value) {
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
// atomic_ptr_uint_u* expand_table(wtf_table_t* t, atomic_ptr_uint_u* expanded_node, size_t r) {
//    uintptr_t node = atomic_load_explicit(&expanded_node->as_uint, memory_order_acquire);
//    atomic_ptr_uint_u* new_array = malloc(sizeof(atomic_ptr_uint_u) * t->arr_sz_);
//    memset(new_array, 0, sizeof(atomic_ptr_uint_u) * t->arr_sz_);
//    printf("exp: new array allocated: %p\n", new_array);
//
//    // calc expanded node idx in new array
//    uint8_t expanded_node_new_pos = (expanded_node->as_ptr->hash_ >> (t->bits_shift + r)) & (t->arr_sz_ - 1);
//    printf("exp: expanded node new pos: %u\n", expanded_node_new_pos);
//    new_array[expanded_node_new_pos].as_ptr = expanded_node->as_ptr;
//
//    mark_as_array((void**)&new_array);
//    printf("exp: new array is marked: %p\n", new_array);
//
//    // replace node with array
//    if (atomic_compare_exchange_strong(&expanded_node->as_uint, &node, (uintptr_t)new_array)) {
//        return new_array;
//    } else {
//        // other thread has inserted array in this node already
//        if (is_array_node(&expanded_node->as_uint)) {
//            // the node was already expanded, free stuff created above
//            free(new_array);
//            return expanded_node;
//        }
//    }
//    return NULL;
//}

// Insert key-value pair it table.
// here key is precalculated hash of a real key
// If the table holds the key, value overwrites the previous value, and wtf_table_insert returns the previous value.
// Otherwise, key value pair is added to the table, and wtf_table_insert returns NULL
void* wtf_table_insert(wtf_table_t* t, hash_t hash, const void* value) {
    atomic_node_ptr_t node, new_node;
    new_node = allocate_node(hash, value);
    printf("ins: node allocated at %p\n", new_node);

    if (NULL == new_node) {
        return NULL;
    }
    atomic_node_ptr_t* local = t->head_; // start from head
    uint8_t fail_count = 0;

    for (size_t r = 0; r < BITS_COUNT(hash); r += t->bits_shift) {
        // TODO: 10th r should be 4 bits length
        uint8_t pos = hash & (t->arr_sz_ - 1); // position in local(current) array node
        printf("ins: hash: %zu, pos in local: %u\n", hash, pos);
        hash >>= t->bits_shift;
        fail_count = 0;

        for (;;) {
            if (fail_count > t->max_fail_count) {
                mark_atomic_node(&local[pos]);
            }
            node = local[pos];

            printf("ins: candidate at %u pos is %p\n", pos, node);
            if (is_array(&node)) {
                local = get_unmarked_array(node);
                break; // for
            } else if (is_atomic_node_marked(&node)) {
                // TODO:   local = expand_table(local, pos, node, r);
                break;
            } else if (NULL == node) {
                // TODO: explicit cmp func
                if (atomic_compare_exchange_strong((atomic_uintptr_t*)local[pos], (uintptr_t*)node,
                                                   (uintptr_t)new_node)) {
                    printf("ins: %p is inserted at %u, level: %lu\n", new_node, pos, r / t->bits_shift);
                    return NULL; // node added
                } else {
                    // TBD
                }
            } else {
                // do I need atomic pointer here?
                if (node->hash_ == new_node->hash_) {

                } else {
                    //                    printf("ins: expand %p\n", local);
                    //                    atomic_node_ptr_t* expanded = expand_table(t, &local[pos], r);
                    //                    printf("ins: expanded %p, in table %p\n", expanded, local[pos]);
                    //
                    //                    if (!is_array(expanded)) {
                    //                        ++fail_count;
                    //                    } else {
                    //                        //                    unmark_array_node_ex((void**)&expanded);
                    //                        //                    printf("ins: unmarked expanded %p\n", expanded);
                    //                        local = expanded;
                    //                        printf("ins: set local to %p\n", local);
                    //                        break; // inner for
                    //                    }
                }
            }
        }
    }
    return NULL;
}

// Return value associated with the key, if the table doesn't hold the key, return NULL
const void* wtf_table_find(wtf_table_t* t, size_t hash) {
    atomic_node_ptr_t* local = t->head_;
    node_ptr_t* node;

    size_t local_hash = hash;
    uint8_t pos;
    printf("fnd: ==> find %lu hash\n", hash);

    for (size_t r = 0; r < BITS_COUNT(hash); r += t->bits_shift) {
        pos = local_hash & (t->arr_sz_ - 1);
        // printf("fnd: pos in local: %u\n", pos);
        local_hash >>= t->bits_shift;
        // printf("fnd: attempt to unmark node: %p\n", local[pos].as_ptr);
        node = get_unmarked_node(&local[pos]);
        // printf("fnd: unmarked: %p\n", node.as_ptr);

        if (is_array(node)) {
            printf("fnd: array node %p in pos: %u, level: %lu\n", node, pos, r / t->bits_shift);
            local = &local[pos];
            unmark_array((void**)&local);
            printf("fnd: unmarked array: %p\n", local);
        } else {
            printf("fnd: regular node %p in pos: %u, level: %lu\n", node, pos, r / t->bits_shift);
            if (NULL == node) {
                printf("fnd: node is empty\n");
                return NULL;
            } else if ((*node)->hash_ == hash) {
                printf("fnd: hash matched: %lu is found\n", hash);
                return (*node)->value_;
            } else {
                printf("fnd: hash mismatched: requested: %lu, found: %lu\n", hash, (*node)->hash_);
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
