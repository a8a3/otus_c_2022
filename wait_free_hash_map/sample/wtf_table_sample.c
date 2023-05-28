#define _GNU_SOURCE

#define NDEBUG

#include <stdio.h>
#include <threads.h>
#include "wtf_table.h"
#include <unistd.h>

atomic_bool run = false;

typedef struct {
    wtf_table_t* t;
    int begin;
    int end;
} table_ins_data_t;

char* values[] = {"0"};
int inserter(void* data) {
    table_ins_data_t* d = (table_ins_data_t*)data;
    pid_t tid = gettid();
    printf("tid: %d started\n", tid);
    while (!run)
        ;
    for (int i = d->begin; i <= d->end; ++i) {
        wtf_table_insert(d->t, i, values[0]);
    }
    return 0;
}

void print(atomic_node_ptr_t n, void* user_data) {
    (void)user_data;
    if (is_atomic_array(n)) return;
    printf("hash: %lu, val: %s\n", n->hash_, (char*)n->value_);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    printf("===> wtf_table sample\n");
    wtf_table_t* tbl = wtf_table_create();
    assert(tbl);

    table_ins_data_t thrd_data_0 = {.begin = 0, .end = 100000L, .t = tbl};
    table_ins_data_t thrd_data_1 = {.begin = 0, .end = 100000L, .t = tbl};
    table_ins_data_t thrd_data_2 = {.begin = 0, .end = 100000L, .t = tbl};
    thrd_t thrds[3];
    thrd_create(&thrds[0], inserter, &thrd_data_0);
    thrd_create(&thrds[1], inserter, &thrd_data_1);
    thrd_create(&thrds[2], inserter, &thrd_data_2);

    run = true;

    int res;
    thrd_join(thrds[0], &res);
    thrd_join(thrds[1], &res);
    thrd_join(thrds[2], &res);

    wtf_table_for_each(tbl, print, NULL);
    size_t cnt = wtf_table_count(tbl);
    printf("elems count: %lu\n", cnt);
    wtf_table_destroy(&tbl);
    return 0;
}
