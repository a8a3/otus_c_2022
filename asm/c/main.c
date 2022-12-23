#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const char* int_format = "%ld ";
const char* empty_str = "";
long data[] = {4, 8, 15, 16, 23, 42};
size_t data_length = sizeof data / sizeof data[0];

struct node {
    long val;
    struct node* next;
};

struct node* add_element(struct node* n, long val) {
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    if (NULL == new_node)
        abort();
    new_node->val = val;
    new_node->next = n;
    return new_node;
}

void remove_list(struct node* n) {
    struct node* next = NULL;
    while (n) {
        next = n->next;
        free(n);
        n = next;
    }
}

void print_int(long val) {
    printf(int_format, val);
    fflush(NULL);
}
typedef void (*print_int_ptr)(long);

void m(struct node* n, print_int_ptr print) {
    while (n) {
        print(n->val);
        n = n->next;
    }
}

bool is_odd(long val) { return val & 1; }
typedef bool (*is_odd_ptr)(long val);

struct node* f(struct node* src, is_odd_ptr filter) {
    struct node* res = NULL;
    while (src) {
        if (filter(src->val)) {
            res = add_element(res, src->val);
        }
        src = src->next;
    }
    return res;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    int num = (int)data_length - 1;
    struct node* list = NULL;
    do {
        list = add_element(list, data[num]);
    } while (--num >= 0);

    m(list, print_int);
    puts(empty_str);

    struct node* odd_list = f(list, is_odd);
    m(odd_list, print_int);
    puts(empty_str);

    remove_list(list);
    remove_list(odd_list);
}
