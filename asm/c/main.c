#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const char* int_format = "%ld ";
const char* empty_str = "";
long data[] = {4, 8, 15, 16, 23, 42};
size_t data_length = sizeof data / sizeof data[0];

long* add_element(int val, long* node) {
    long* new_node = (long*)malloc(16);
    if (NULL == new_node) abort();
    *new_node = val;
    *(new_node + 1) = (long)node;
    return new_node;
}

void remove_list(long* node) {
    long* next = NULL;
    while (node) {
        next = (long*)(*(node + 1));
        free(node);
        node = next;
    }
}

void print_int(long val) {
    printf(int_format, val);
    fflush(NULL);
}
typedef void(*print_int_ptr)(long);

void m(long* node, print_int_ptr print) {
    while(node) {
        print(*node);
        node = (long*)(*(node + 1));
    }
}

bool is_odd(long val) {
    return val & 1;
}
typedef bool(*is_odd_ptr)(long val);

long* f(long* src, long* dst, is_odd_ptr filter) {
    while(src) {
       if (filter(*src)) {
           dst = add_element(*src, dst);
       }
       src = (long*)(*(src + 1));
    }
    return dst;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)**argv;

    int num = (int)data_length - 1;
    long* list = NULL;
    do {
        list = add_element(data[num], list);
    } while(--num >= 0);

    m(list, print_int);
    puts(empty_str);

    long* odd_list = f(list, NULL, is_odd);
    m(odd_list, print_int);
    puts(empty_str);

    remove_list(list);
    remove_list(odd_list);
}

