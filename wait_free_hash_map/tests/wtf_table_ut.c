#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// TODO: turn on
// #define UNIT_TESTING // this define enable memory leaks checking awailable in cmocka framework
//
#include <cmocka.h>
#include <wtf_table.h>

int g_val = 42;
// some statements I need to be aware of before implementation
static void wtf_table_statements(void** state) { // NOLINT
    // expected that all pointers are aligned
    assert_true(_Alignof(int*) >= 4);
    int* p = &g_val;

    // cast to integer
    uintptr_t i = (uintptr_t)p;
    print_message("original: %p, casted to int     : 0x%lx\n", p, i);
    print_message("original: %p, casted back to ptr: %p\n", p, (int*)i); // NOLINT

    assert_ptr_equal(p, (int*)i); // NOLINT: Integer to pointer cast pessimizes optimization opportunities

    // 2 LSB are not used
    assert_true((i & 3U) == 0U);

    // add tag to integer pointer representation
    i = i | 1U;
    print_message("original: %p, casted back marked: %p\n", p, (int*)i); // NOLINT

    // cast back to pointer
    int* pt = (int*)i; // NOLINT: Integer to pointer cast pessimizes optimization opportunities

    // cast back to integer and remove tag
    uintptr_t iu = (uintptr_t)pt & ~((uintptr_t)1U);

    // cast back to a pointer
    int* pu = (int*)iu; // NOLINT: Integer to pointer cast pessimizes optimization opportunities
    assert_ptr_equal(p, pu);

    *pu = 0;
    assert_int_equal(g_val, 0);
}

typedef _Atomic(int*) atomic_iptr_t; // i.e atomic pointer to non- atomic int
typedef union {
    atomic_iptr_t as_ptr;
    atomic_uintptr_t as_uint;
} atomic_iptr_uint_u;

static void wtf_table_atomic_statements(void** state) { // NOLINT
    // expected that all atomic pointers are aligned (4 bytes boundary on x32 systems)
    assert_true(_Alignof(atomic_iptr_uint_u) >= 4);
    assert_true(_Alignof(atomic_iptr_t) >= 4);
    assert_true(_Alignof(atomic_uintptr_t) >= 4);

    atomic_iptr_uint_u u;
    u.as_ptr = &g_val;

    print_message("atomic pointer representation: %p, atomic uint representation: 0x%lx\n", u.as_ptr, u.as_uint);
    // 2 LSB are not used
    assert_true((u.as_uint & 3U) == 0U);

    // add tag to LSB of integer pointer representation
    atomic_fetch_or(&u.as_uint, (uintptr_t)(1U));
    print_message("marked  : %p, 0x%lx\n", u.as_ptr, u.as_uint);

    // remove tag from LSB
    atomic_fetch_and(&u.as_uint, ~((uintptr_t)(1U)));
    print_message("unmarked: %p, 0x%lx\n", u.as_ptr, u.as_uint);
    *u.as_ptr = 0;
    assert_int_equal(g_val, 0);
}

static void wtf_table_nodes_marking(void** state) { // NOLINT
    uintptr_t initial_val = 0xFFFFFFFC;
    atomic_uintptr_t val = initial_val;
    assert_false(is_marked_node(val));

    mark_data_node(&val);
    assert_true(is_marked_node(val));
    assert_false(val == initial_val);

    atomic_uintptr_t unmarked = unmark_data_node(val);
    assert_false(is_marked_node(unmarked));

    // rest bits are fine as well
    assert_true(unmarked == initial_val);
}

static void wtf_table_creation_deletion(void** state) { // NOLINT
    wtf_table_t* t = wtf_table_create(64);
    assert_ptr_not_equal(t, NULL);
    wtf_table_destroy(&t);
    assert_ptr_equal(t, NULL);
}

static void wtf_table_insertion(void** state) { // NOLINT
    wtf_table_t* t = wtf_table_create(64);
    const void* res = wtf_table_insert(t, 0, "value");
    assert_ptr_equal(res, NULL);
    res = wtf_table_find(t, 0);
    assert_string_equal(res, "value");
    res = wtf_table_insert(t, 1, "value1");
    res = wtf_table_insert(t, 2, "value2");
    res = wtf_table_insert(t, 3, "value3");
    res = wtf_table_insert(t, 4, "value4");

    res = wtf_table_find(t, 1);
    assert_string_equal(res, "value1");
    res = wtf_table_find(t, 2);
    assert_string_equal(res, "value2");
    res = wtf_table_find(t, 3);
    assert_string_equal(res, "value3");
    res = wtf_table_find(t, 4);
    assert_string_equal(res, "value4");

    wtf_table_destroy(&t);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(wtf_table_statements), cmocka_unit_test(wtf_table_atomic_statements),
        cmocka_unit_test(wtf_table_nodes_marking), cmocka_unit_test(wtf_table_creation_deletion),
        cmocka_unit_test(wtf_table_insertion)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
