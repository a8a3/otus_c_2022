#include "logger.h"
#include <assert.h>

LOGGER g_log;

void bad() { LOG_ERR(g_log, "error: %d", 42); }
void bar() { bad(); }
void foo() { bar(); }

void test_file_based_log();

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    g_log = LOG_OPEN_STDOUT();

    LOG_INFO(g_log, "app start");
    LOG_DEBUG(g_log, "test. string=%s", "some string");
    LOG_WARN(g_log, "warning!");
    LOG_INFO(g_log, "app end");

    foo();

    test_file_based_log();

    LOG_CLOSE(&g_log);
    assert(NULL == g_log);
}

void test_file_based_log() {
    LOGGER l = LOG_OPEN_FILE("test.log");
    assert(l);
    LOG_INFO(l, "log init");
    LOG_WARN(l, "warning!");
    LOG_ERR(l, "error: %d", 42);
    LOG_WARN(l, "one more warning!");
    LOG_ERR(l, "error again: %d", 43);
    LOG_INFO(l, "log closed");
    LOG_CLOSE(&l);
    assert(NULL == l);
}
