#include "wtf_table.h"
#include <stdio.h>
#include <threads.h>

atomic_bool run = false;

typedef struct {
    wtf_table_t* t;
    int begin;
    int end;
} table_ins_data_t;

char* values[] = {"0", "1", "2", "3", "4"};
int inserter(void* data) {
    table_ins_data_t* d = (table_ins_data_t*)data;
    //    while (!run)
    //        ;
    for (int i = d->begin; i <= d->end; ++i) {
        bool res = wtf_table_insert(d->t, i, values[i]);
        printf("value: %s, %s\n", values[i], (res ? "updated" : "inserted"));
    }
    return 0;
}

void print(atomic_node_ptr_t n, void* user_data) {
    (void)user_data;
    printf("n hash: %lu, n val: %s\n", n->hash_, (char*)n->value_);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    printf("===> wtf_table sample\n");
    wtf_table_t* t = wtf_table_create();
    assert(t);

    table_ins_data_t thrd_data_1 = {.begin = 0, .end = 2, .t = t};
    table_ins_data_t thrd_data_2 = {.begin = 2, .end = 3, .t = t};
    thrd_t thrds[2];
    thrd_create(&thrds[0], inserter, &thrd_data_1);
    thrd_create(&thrds[1], inserter, &thrd_data_2);

    // run = true;

    int res;
    thrd_join(thrds[0], &res);
    thrd_join(thrds[1], &res);

    wtf_table_for_each(t, print, NULL);
    wtf_table_destroy(&t);
    return 0;
}
