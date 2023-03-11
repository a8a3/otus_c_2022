#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define UNIT_TESTING // this define enable memory leaks checking awailable in cmocka framework
#include <cmocka.h>

static void dummy_test(void** state) { // NOLINT
    assert_uint_equal(42, 42);
}

// static void pirsons_hash_test(void **state) { //NOLINT
//     uint32_t cat_hash = pirsons_hash("cat", 3);
//     uint32_t dog_hash = pirsons_hash("dog", 3);
//     assert_uint_not_equal(cat_hash, dog_hash);
//
//     assert_uint_equal(cat_hash, pirsons_hash("cat", 3));
//     assert_uint_equal(dog_hash, pirsons_hash("dog", 3));
// }
//
// static void hash_map_creation_test(void **state) { // NOLINT
//     hash_map* m = create_hash_map(8);
//     assert_non_null(m);
//     assert_uint_equal(m->capacity, 8);
//     assert_uint_equal(m->size, 0);
//     remove_hash_map(&m);
//     assert_ptr_equal(m, NULL);
// }
//
// static void hash_map_basic_operations_test(void **state) { // NOLINT
//     hash_map* m = create_hash_map(8);
//
//     const char* key = "test";
//     const size_t key_len = strlen(key);
//     const int val = 42;
//
//     // insert new node
//     node* n = hash_map_insert(m, key, key_len);
//     assert_ptr_not_equal(n, NULL);
//     assert_int_equal(n->val, 0);
//     n->val = val;
//
//     assert_uint_equal(n->key_len, key_len);
//     assert_string_equal(n->key, key);
//     assert_uint_equal(m->size, 1);
//
//     // get non-existent node
//     const char* bad_key = "bad_key";
//     n = hash_map_get_node(m, bad_key, strlen(bad_key));
//     assert_ptr_equal(n, NULL);
//
//     // get existing node
//     n = hash_map_get_node(m, key, key_len);
//     assert_ptr_not_equal(n, NULL);
//     assert_int_equal(n->val, val);
//
//     remove_hash_map(&m);
// }
//
// static void hash_map_rehashing_test(void **state) { // NOLINT
//     hash_map* m = create_hash_map(4);
//
//     node* na = hash_map_insert(m, "a", 1);
//     na->val = 128;
//     node* nb = hash_map_insert(m, "b", 1);
//     nb->val = 256;
//     assert_uint_equal(m->capacity, 4);
//     assert_uint_equal(m->size, 2);
//
//     node* cur_nodes_ptr = m->nodes;
//     node* nc = hash_map_insert(m, "c", 1);
//     nc->val = 512;
//     assert_ptr_not_equal(cur_nodes_ptr, m->nodes);
//     assert_uint_equal(m->capacity, 8);
//     assert_uint_equal(m->size, 3);
//
//     // na, nb nodes were invalidated
//     node* new_na = hash_map_get_node(m, "a", 1);
//     assert_ptr_not_equal(new_na, NULL);
//     assert_ptr_not_equal(new_na, na);
//     assert_uint_equal(new_na->val, 128);
//
//     node* new_nb = hash_map_get_node(m, "b", 1);
//     assert_ptr_not_equal(new_nb, NULL);
//     assert_ptr_not_equal(new_nb, nb);
//     assert_uint_equal(new_nb->val, 256);
//
//     remove_hash_map(&m);
// }
//
// static void words_counting_test(void **state) { // NOLINT
//     FILE* tmp_file = tmpfile();
//     fputs("one\n", tmp_file);
//     fputs("два два\n", tmp_file);
//     fputs("three three three\n", tmp_file);
//     fputs("четыре,четыре,четыре,четыре\n", tmp_file);
//     rewind(tmp_file);
//
//     hash_map* hm = create_hash_map(HASH_MAP_DEFAULT_SIZE);
//     assert_ptr_not_equal(hm, NULL);
//
//     hash_map_init(hm, tmp_file);
//
//     node* n1 = hash_map_get_node(hm, "one", strlen("one"));
//     assert_ptr_not_equal(n1, NULL);
//     assert_int_equal(n1->val, 1);
//
//     node* n2 = hash_map_get_node(hm, "два", strlen("два"));
//     assert_ptr_not_equal(n2, NULL);
//     assert_int_equal(n2->val, 2);
//
//     node* n3 = hash_map_get_node(hm, "three", strlen("three"));
//     assert_ptr_not_equal(n3, NULL);
//     assert_int_equal(n3->val, 3);
//
//     node* n4 = hash_map_get_node(hm, "четыре", strlen("четыре"));
//     assert_ptr_not_equal(n4, NULL);
//     assert_int_equal(n4->val, 4);
//
//     remove_hash_map(&hm);
//     assert_ptr_equal(hm, NULL);
// }
//
// static void words_delimeters_test(void **state) { // NOLINT
//     FILE* tmp_file = tmpfile();
//     fputs("one two\nthree,four.five?six!seven;eight", tmp_file);
//     rewind(tmp_file);
//
//     hash_map* hm = create_hash_map(HASH_MAP_DEFAULT_SIZE);
//     hash_map_init(hm, tmp_file);
//
//     node* n1 = hash_map_get_node(hm, "one", 3);
//     assert_ptr_not_equal(n1, NULL);
//     assert_int_equal(n1->val, 1);
//
//     node* n2 = hash_map_get_node(hm, "two", 3);
//     assert_ptr_not_equal(n2, NULL);
//     assert_int_equal(n2->val, 1);
//
//     node* n3 = hash_map_get_node(hm, "three", 5);
//     assert_ptr_not_equal(n3, NULL);
//     assert_int_equal(n3->val, 1);
//
//     node* n4 = hash_map_get_node(hm, "four", 4);
//     assert_ptr_not_equal(n4, NULL);
//     assert_int_equal(n4->val, 1);
//
//     node* n5 = hash_map_get_node(hm, "five", 4);
//     assert_ptr_not_equal(n5, NULL);
//     assert_int_equal(n5->val, 1);
//
//     node* n6 = hash_map_get_node(hm, "six", 3);
//     assert_ptr_not_equal(n6, NULL);
//     assert_int_equal(n6->val, 1);
//
//     node* n7 = hash_map_get_node(hm, "seven", 5);
//     assert_ptr_not_equal(n7, NULL);
//     assert_int_equal(n7->val, 1);
//
//     remove_hash_map(&hm);
// }

int main(void) {
    const struct CMUnitTest tests[] = {cmocka_unit_test(dummy_test)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
