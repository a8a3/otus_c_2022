#include <stdio.h>
#include <threads.h>

int f(void* data) {
    printf("worker idx: %d\n", *((int*)data));
    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("valgrind, tsan test\n");

    thrd_t thrds[2];
    int d[2] = {1, 2};
    thrd_create(&thrds[0], f, &d[0]);
    thrd_create(&thrds[1], f, &d[1]);

    int res;
    thrd_join(thrds[0], &res);
    thrd_join(thrds[1], &res);
    return 0;
}
