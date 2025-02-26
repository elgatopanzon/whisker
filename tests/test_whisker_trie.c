/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_trie
 * @created     : Thursday Feb 06, 2025 15:15:12 CST
 */

#include "whisker_trie.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

void whisker_trie_test_create_cat_tree(whisker_trie** trie_root)
{
	// create trie root node
	whisker_trie_create_node(trie_root);

	// create 3 trie child nodes
	whisker_trie* trie_a;
	whisker_trie_create_child_node(*trie_root, 'a', &trie_a);

	whisker_trie* trie_b;
	whisker_trie_create_child_node(*trie_root, 'b', &trie_b);

	// trie tree for "cat"
	whisker_trie* trie_c;
	whisker_trie_create_child_node(*trie_root, 'c', &trie_c);
	whisker_trie* trie_ca;
	whisker_trie_create_child_node(trie_c, 'a', &trie_ca);
	whisker_trie* trie_cat;
	whisker_trie_create_child_node(trie_ca, 't', &trie_cat);
	trie_cat->value = "cat";
}

START_TEST(test_whisker_trie_create)
{
	// create trie root node
	whisker_trie* trie_root;
	whisker_trie_create_node(&trie_root);

	// free node from root
	whisker_trie_free_node(trie_root, false);
}
END_TEST

START_TEST(test_whisker_trie_create_node_tree)
{
	// create trie root node
	whisker_trie* trie_root;
	whisker_trie_test_create_cat_tree(&trie_root);

	// free node from root (should free all children too)
	whisker_trie_free_node(trie_root, false);
}
END_TEST

START_TEST(test_whisker_trie_search_node_by_key)
{
	// create trie root node
	whisker_trie* trie_root;
	whisker_trie_test_create_cat_tree(&trie_root);

	// search for the prefix match "ca" node
	whisker_trie* ca_node;
	E_WHISKER_TRIE errCa = whisker_trie_search_node_str(trie_root, "ca", &ca_node);

	// verify error code
	ck_assert_int_eq(errCa, E_WHISKER_TRIE_OK);

	// verify value is empty
	ck_assert(ca_node != NULL && ca_node->value == NULL);

	// search for the "cat" node
	whisker_trie* cat_node;
	E_WHISKER_TRIE errCat = whisker_trie_search_node_str(trie_root, "cat", &cat_node);

	// verify error code
	ck_assert_int_eq(errCat, E_WHISKER_TRIE_OK);

	// verify the value of the node matches "cat"
	ck_assert_str_eq("cat", cat_node->value);

	// search for a partially missing "cut" node
	whisker_trie* cut_node;
	E_WHISKER_TRIE errCut = whisker_trie_search_node_str(trie_root, "cut", &cut_node);

	// verify error code
	ck_assert_int_eq(errCut, E_WHISKER_TRIE_SEARCH_MISSING_NODE);

	// search for a missing "dog" node
	whisker_trie* dog_node;
	E_WHISKER_TRIE errDog = whisker_trie_search_node_str(trie_root, "dog", &dog_node);

	// verify error code
	ck_assert_int_eq(errDog, E_WHISKER_TRIE_SEARCH_MISSING_ALL);

	// free node from root (should free all children too)
	whisker_trie_free_node(trie_root, false);
}
END_TEST

START_TEST(test_whisker_trie_search_value_by_key)
{
	// create trie root node
	whisker_trie* trie_root;
	whisker_trie_test_create_cat_tree(&trie_root);

	// search for the "cat" key's value
	char* cat_value;
	E_WHISKER_TRIE errCat = whisker_trie_search_value_str(trie_root, "cat", &cat_value);

	// verify error code
	ck_assert_int_eq(errCat, E_WHISKER_TRIE_OK);

	// verify the value of the node matches "cat"
	ck_assert_str_eq("cat", cat_value);

	// search for the partial match "ca" key's value
	char* ca_value;
	E_WHISKER_TRIE errCa = whisker_trie_search_value_str(trie_root, "ca", &ca_value);

	// verify error code
	ck_assert_int_eq(errCa, E_WHISKER_TRIE_SEARCH_MISSING_VALUE);

	// free node from root (should free all children too)
	whisker_trie_free_node(trie_root, false);
}
END_TEST

START_TEST(test_whisker_trie_set_value)
{
	// create trie root node
	whisker_trie* trie_root;
	whisker_trie_test_create_cat_tree(&trie_root);

	// search for the "cat" key's value
	char* cat_value;
	E_WHISKER_TRIE errCat = whisker_trie_search_value_str(trie_root, "cat", &cat_value);

	// verify error code
	ck_assert_int_eq(errCat, E_WHISKER_TRIE_OK);

	// verify the value of the node matches "cat"
	ck_assert_str_eq("cat", cat_value);

	// set value to dog
	whisker_trie_set_value_str(&trie_root, "dog", "dog");
	
	// search for the "dog" key's value
	char* dog_value;
	E_WHISKER_TRIE errDog = whisker_trie_search_value_str(trie_root, "dog", &dog_value);

	// verify error code
	ck_assert_int_eq(errDog, E_WHISKER_TRIE_OK);

	// verify the value of the node matches "cat"
	ck_assert_str_eq("dog", dog_value);

	// free node from root (should free all children too)
	whisker_trie_free_node(trie_root, false);
}
END_TEST

void whisker_trie_test_set_int(whisker_trie* root)
{
	// set value to an int
	int* val = malloc(sizeof(int));
	*val = 123;

	whisker_trie_set_value_str(&root, "dog", val);
}

START_TEST(test_whisker_trie_set_value_int)
{
	// create trie root node
	whisker_trie* trie_root;
	whisker_trie_test_create_cat_tree(&trie_root);

	whisker_trie_test_set_int(trie_root);
	
	// search for the "dog" key's value
	int* dog_value;
	E_WHISKER_TRIE errDog = whisker_trie_search_value_str(trie_root, "dog", &dog_value);

	// verify error code
	ck_assert_int_eq(errDog, E_WHISKER_TRIE_OK);

	// verify the value of the node matches "cat"
	ck_assert_int_eq(123, *dog_value);

	// free node from root (should free all children too)
	whisker_trie_free_node(trie_root, false);
	free(dog_value);
}
END_TEST

START_TEST(test_whisker_trie_set_value_uint64_key)
{
	// create trie root node
	whisker_trie* trie_root;
	whisker_trie_create_node(&trie_root);

	// set some large keys
	uint64_t uintkey_1 = 4294967296;
	whisker_trie_set_value(&trie_root, &uintkey_1, sizeof(uint64_t), &trie_root);

	uint64_t uintkey_2 = 8589934593;
	whisker_trie_set_value(&trie_root, &uintkey_2, sizeof(uint64_t), &trie_root);

	uint64_t uintkey_3 = 1095216660481;
	whisker_trie_set_value(&trie_root, &uintkey_3, sizeof(uint64_t), &trie_root);

	// search for the key's values
	whisker_trie* key1_value;
	whisker_trie_search_value_f(trie_root, &uintkey_1, sizeof(uint64_t), (void**)&key1_value);
	whisker_trie* key2_value;
	whisker_trie_search_value_f(trie_root, &uintkey_2, sizeof(uint64_t), (void**)&key2_value);
	whisker_trie* key3_value;
	whisker_trie_search_value_f(trie_root, &uintkey_3, sizeof(uint64_t), (void**)&key3_value);

	// verify the value of the node 
	ck_assert_ptr_eq(&trie_root, key1_value);
	ck_assert_ptr_eq(&trie_root, key2_value);
	ck_assert_ptr_eq(&trie_root, key3_value);

	// free node from root (should free all children too)
	whisker_trie_free_node(trie_root, false);
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
