#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <setjmp.h>

#define UNIT_TESTING // this define allows to enable memory leaks checking awailable in cmocka framework
#include <cmocka.h>
#include <hash_map.h>
#include <frequency_dictionary.h>

static void pirsons_hash_test(void **state) { //NOLINT
    uint32_t cat_hash = pirsons_hash("cat", 3);
    uint32_t dog_hash = pirsons_hash("dog", 3);
    assert_uint_not_equal(cat_hash, dog_hash);

    assert_uint_equal(cat_hash, pirsons_hash("cat", 3));
    assert_uint_equal(dog_hash, pirsons_hash("dog", 3));
}

static void hash_map_creation_test(void **state) { // NOLINT
    hash_map* m = create_hash_map(8);
    assert_non_null(m);
    assert_uint_equal(m->capacity, 8);
    assert_uint_equal(m->size, 0);
    remove_hash_map(&m);
    assert_ptr_equal(m, NULL);
}

static void hash_map_basic_operations_test(void **state) { // NOLINT
    hash_map* m = create_hash_map(8);

    const char* key = "test";
    const size_t key_len = strlen(key);
    const int val = 42;

    // insert new node
    node* n = hash_map_insert(m, key, key_len);
    assert_ptr_not_equal(n, NULL);
    assert_int_equal(n->val, 0);
    n->val = val;

    assert_uint_equal(n->key_len, key_len);
    assert_string_equal(n->key, key);
    assert_uint_equal(m->size, 1);

    // get non-existent node
    const char* bad_key = "bad_key";
    n = hash_map_get_node(m, bad_key, strlen(bad_key));
    assert_ptr_equal(n, NULL);

    // get existing node
    n = hash_map_get_node(m, key, key_len);
    assert_ptr_not_equal(n, NULL);
    assert_int_equal(n->val, val);

    remove_hash_map(&m);
}

static void hash_map_rehashing_test(void **state) { // NOLINT
    hash_map* m = create_hash_map(4);
    
    node* na = hash_map_insert(m, "a", 1);
    na->val = 128;
    node* nb = hash_map_insert(m, "b", 1);
    nb->val = 256;
    assert_uint_equal(m->capacity, 4);
    assert_uint_equal(m->size, 2);

    node* cur_nodes_ptr = m->nodes;
    node* nc = hash_map_insert(m, "c", 1);
    nc->val = 512;
    assert_ptr_not_equal(cur_nodes_ptr, m->nodes);
    assert_uint_equal(m->capacity, 8);
    assert_uint_equal(m->size, 3);

    // na, nb nodes were invalidated
    node* new_na = hash_map_get_node(m, "a", 1);
    assert_ptr_not_equal(new_na, NULL);
    assert_ptr_not_equal(new_na, na);
    assert_uint_equal(new_na->val, 128);

    node* new_nb = hash_map_get_node(m, "b", 1);
    assert_ptr_not_equal(new_nb, NULL);
    assert_ptr_not_equal(new_nb, nb);
    assert_uint_equal(new_nb->val, 256);

    remove_hash_map(&m);
}

static void frequency_dictionary_test(void **state) { // NOLINT
    FILE* tmp_file = tmpfile();
    fputs("one\n", tmp_file);
    fputs("two two\n", tmp_file);
    fputs("three three three\n", tmp_file);
    rewind(tmp_file);

    frequency_dictionary* fd = create_frequency_dictionary(tmp_file);
    assert_ptr_not_equal(fd, NULL);

    node* n1 = hash_map_get_node(fd->words_map, "one", 3);
    assert_ptr_not_equal(n1, NULL);
    assert_int_equal(n1->val, 1);

    node* n2 = hash_map_get_node(fd->words_map, "two", 3);
    assert_ptr_not_equal(n2, NULL);
    assert_int_equal(n2->val, 2);

    node* n3 = hash_map_get_node(fd->words_map, "three", 5);
    assert_ptr_not_equal(n3, NULL);
    assert_int_equal(n3->val, 3);

    remove_frequency_dictionary(&fd);
    assert_ptr_equal(fd, NULL);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(pirsons_hash_test),
        cmocka_unit_test(hash_map_creation_test),
        cmocka_unit_test(hash_map_basic_operations_test),
        cmocka_unit_test(hash_map_rehashing_test),
        cmocka_unit_test(frequency_dictionary_test)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}