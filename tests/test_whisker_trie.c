/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_trie
 * @created     : Thursday Feb 06, 2025 15:15:12 CST
 */

#include "whisker_trie.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

w_trie_node* whisker_trie_test_create_cat_tree()
{
	// create trie root node
	w_trie_node *root = whisker_mem_xcalloc_t(1, whisker_trie);

	// create 3 trie child nodes
	w_trie_node* trie_a = whisker_trie_create_child_node(root, 'a');

	w_trie_node* trie_b = whisker_trie_create_child_node(root, 'b');

	// trie tree for "cat"
	w_trie_node* trie_c = whisker_trie_create_child_node(root, 'c');
	w_trie_node* trie_ca = whisker_trie_create_child_node(trie_c, 'a');
	w_trie_node* trie_cat = whisker_trie_create_child_node(trie_ca, 't');
	trie_cat->value = "cat";

	return root;
}

START_TEST(test_whisker_trie_create)
{
	// create trie root node
	w_trie_node* trie_root = whisker_mem_xcalloc_t(1, whisker_trie);

	// free node from root
	w_trie_free_nodes(trie_root);
	free(trie_root);
}
END_TEST

START_TEST(test_whisker_trie_create_node_tree)
{
	// create trie root node
	w_trie_node* trie_root = whisker_trie_test_create_cat_tree();

	// free node from root (should free all children too)
	w_trie_free_nodes(trie_root);
	free(trie_root);
}
END_TEST

START_TEST(test_whisker_trie_search_node_by_key)
{
	// create trie root node
	w_trie_node* trie_root = whisker_trie_test_create_cat_tree();

	// search for the prefix match "ca" node
	w_trie_node* ca_node = whisker_trie_search_node_str(trie_root, "ca");

	// verify value is empty
	ck_assert(ca_node != NULL && ca_node->value == NULL);

	// search for the "cat" node
	w_trie_node* cat_node = whisker_trie_search_node_str(trie_root, "cat");

	// verify the value of the node matches "cat"
	ck_assert_str_eq("cat", cat_node->value);

	// search for a partially missing "cut" node
	w_trie_node* cut_node = whisker_trie_search_node_str(trie_root, "cut");

	ck_assert_ptr_eq(NULL, cut_node);

	// search for a missing "dog" node
	w_trie_node* dog_node = whisker_trie_search_node_str(trie_root, "dog");

	ck_assert_ptr_eq(NULL, dog_node);

	// free node from root (should free all children too)
	w_trie_free_nodes(trie_root);
	free(trie_root);
}
END_TEST

START_TEST(test_whisker_trie_search_value_by_key)
{
	// create trie root node
	w_trie_node* trie_root = whisker_trie_test_create_cat_tree();

	// search for the "cat" key's value
	char* cat_value = whisker_trie_search_value_str(trie_root, "cat");

	// verify the value of the node matches "cat"
	ck_assert_str_eq("cat", cat_value);

	// search for the partial match "ca" key's value
	char* ca_value = whisker_trie_search_value_str(trie_root, "ca");

	ck_assert_ptr_eq(NULL, ca_value);

	// free node from root (should free all children too)
	w_trie_free_nodes(trie_root);
	free(trie_root);
}
END_TEST

START_TEST(test_whisker_trie_set_value)
{
	// create trie root node
	w_trie_node* trie_root = whisker_trie_test_create_cat_tree();

	// search for the "cat" key's value
	char* cat_value = whisker_trie_search_value_str(trie_root, "cat");

	// verify the value of the node matches "cat"
	ck_assert_str_eq("cat", cat_value);

	// set value to dog
	whisker_trie_set_value_str(trie_root, "dog", "dog");
	
	// search for the "dog" key's value
	char* dog_value = whisker_trie_search_value_str(trie_root, "dog");

	// verify the value of the node matches "cat"
	ck_assert_str_eq("dog", dog_value);

	// free node from root (should free all children too)
	w_trie_free_nodes(trie_root);
	free(trie_root);
}
END_TEST

void whisker_trie_test_set_int(w_trie_node* root)
{
	// set value to an int
	int* val = malloc(sizeof(int));
	*val = 123;

	whisker_trie_set_value_str(root, "dog", val);
}

START_TEST(test_whisker_trie_set_value_int)
{
	// create trie root node
	w_trie_node* trie_root = whisker_trie_test_create_cat_tree();

	whisker_trie_test_set_int(trie_root);
	
	// search for the "dog" key's value
	int* dog_value = whisker_trie_search_value_str(trie_root, "dog");

	// verify the value of the node matches "cat"
	ck_assert_int_eq(123, *dog_value);

	// free node from root (should free all children too)
	w_trie_free_nodes(trie_root);
	free(trie_root);
	free(dog_value);
}
END_TEST

START_TEST(test_whisker_trie_set_value_uint64_key)
{
	// create trie root node
	w_trie_node* trie_root = whisker_mem_xcalloc_t(1, whisker_trie);

	// set some large keys
	uint64_t uintkey_1 = 4294967296;
	whisker_trie_set_value(trie_root, &uintkey_1, sizeof(uint64_t), &trie_root);

	uint64_t uintkey_2 = 8589934593;
	whisker_trie_set_value(trie_root, &uintkey_2, sizeof(uint64_t), &trie_root);

	uint64_t uintkey_3 = 1095216660481;
	whisker_trie_set_value(trie_root, &uintkey_3, sizeof(uint64_t), &trie_root);

	// search for the key's values
	w_trie_node* key1_value = whisker_trie_search_value_f(trie_root, &uintkey_1, sizeof(uint64_t));
	w_trie_node* key2_value = whisker_trie_search_value_f(trie_root, &uintkey_2, sizeof(uint64_t));
	w_trie_node* key3_value = whisker_trie_search_value_f(trie_root, &uintkey_3, sizeof(uint64_t));

	// verify the value of the node 
	ck_assert_ptr_eq(&trie_root, key1_value);
	ck_assert_ptr_eq(&trie_root, key2_value);
	ck_assert_ptr_eq(&trie_root, key3_value);

	// free node from root (should free all children too)
	w_trie_free_nodes(trie_root);
	free(trie_root);
}
END_TEST

Suite* whisker_trie_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_trie");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_trie_create);
	tcase_add_test(tc_core, test_whisker_trie_create_node_tree);
	tcase_add_test(tc_core, test_whisker_trie_search_node_by_key);
	tcase_add_test(tc_core, test_whisker_trie_search_value_by_key);
	tcase_add_test(tc_core, test_whisker_trie_set_value);
	tcase_add_test(tc_core, test_whisker_trie_set_value_int);
	tcase_add_test(tc_core, test_whisker_trie_set_value_uint64_key);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_trie_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
