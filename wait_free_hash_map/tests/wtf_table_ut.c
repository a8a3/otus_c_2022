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

    { // non-atomic array marking
        print_message("marking via void ptr convertion\n");
        int* new_int = malloc(sizeof(int));
        print_message(" initial arr: %p\n", new_int);
        assert_false(is_array(new_int));
        mark_as_array((void**)&new_int);
        print_message("  marked arr: %p\n", new_int);
        assert_true(is_array(new_int));

        int* ptr = get_unmarked_array(new_int);
        print_message("unmarked arr: %p\n", ptr);
        assert_false(is_array(ptr));
        assert_true(is_array(new_int));

        unmark_array((void**)&new_int);
        print_message("unmarked arr: %p\n", new_int);
        assert_false(is_array(new_int));
        free(new_int);
    }

    { // atomic node marking
        atomic_node_ptr_t atomic_node = malloc(sizeof(node_t));
        print_message(" initial atomic node: %p\n", atomic_node);
        assert_false(is_atomic_node_marked(&atomic_node));
        mark_atomic_node(&atomic_node);
        print_message("  marked atomic node: %p\n", atomic_node);
        assert_true(is_atomic_node_marked(&atomic_node));

        node_ptr_t* tmp = get_unmarked_node(&atomic_node);
        print_message("unmarked atomic node: %p\n", tmp);
        assert_false(is_node_marked(tmp));
        // origin is still marked
        assert_true(is_atomic_node_marked(&atomic_node));

        unmark_atomic_node(&atomic_node);
        assert_false(is_atomic_node_marked(&atomic_node));
        print_message("unmarked atomic node: %p\n", atomic_node);
        free(atomic_node);
    }
}

static void wtf_table_creation_deletion(void** state) { // NOLINT
    wtf_table_t* t = wtf_table_create(64);
    assert_ptr_not_equal(t, NULL);
    wtf_table_destroy(&t);
    assert_ptr_equal(t, NULL);
}

static void wtf_table_simple_insert_find(void** state) { // NOLINT
    wtf_table_t* t = wtf_table_create(64);
    const void* res = wtf_table_insert(t, 0, "value0");
    assert_ptr_equal(res, NULL);
    res = wtf_table_find(t, 0);
    assert_string_equal(res, "value0");
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

static void wtf_table_insert_find_in_subarrays(void** state) { // NOLINT
    wtf_table_t* t = wtf_table_create(64);
    const void* res = wtf_table_insert(t, 0, "value0");
    assert_ptr_equal(res, NULL);
    // find first node in initial position
    //    res = wtf_table_find(t, 0);
    //    assert_string_equal(res, "value0");

    // 64 has same same 6 LSBs as 0 -> subarray creation expected
    res = wtf_table_insert(t, 64, "value64");
    assert_ptr_equal(res, NULL);

    // find value in subarray
    res = wtf_table_find(t, 64);
    assert_string_equal(res, "value64");

    // find first node in new position on level 1
    res = wtf_table_find(t, 0);
    assert_string_equal(res, "value0");
    wtf_table_destroy(&t);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(wtf_table_statements),
        //  cmocka_unit_test(wtf_table_atomic_statements),
        cmocka_unit_test(wtf_table_nodes_marking),
        // cmocka_unit_test(wtf_table_creation_deletion),
        // cmocka_unit_test(wtf_table_simple_insert_find),
        // cmocka_unit_test(wtf_table_insert_find_in_subarrays)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
