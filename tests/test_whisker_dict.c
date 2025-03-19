/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_dict
 * @created     : Thursday Feb 06, 2025 21:03:41 CST
 */

#include "whisker_dict.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

START_TEST(test_whisker_dict_create)
{
	char** dict;
	whisker_dict_create(&dict, char*, 0);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_add_and_get)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_add_strk(&dict, "cat", &val);

	// get value with key
	int* val_get = whisker_dict_get_strk(dict, "cat");

	// verify gotten value is correct
	ck_assert_int_eq(val, *val_get);

	// get value with missing key
	char* val_get_missing = whisker_dict_get_strk(dict, "dog");

	// verify correct error
	ck_assert(val_get_missing == NULL);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_add_and_get_string)
{
	int* dict;
	whisker_dict_create(&dict, char*, 0);

	// add a value with key
	char* val = "oooooh yeaaa";
	whisker_dict_add_strk(&dict, "string", &val);

	// try to add the same key
	E_WHISKER_DICT add_existing_err = whisker_dict_add_strk(&dict, "string", &val);
	// verify correct error
	ck_assert_int_eq(E_WHISKER_DICT_KEY_EXISTS, add_existing_err);

	// get value with key
	char** val_get = whisker_dict_get_strk(dict, "string");

	// verify gotten value is correct
	ck_assert_str_eq(val, *val_get);

	// get value with missing key
	char* val_get_missing = whisker_dict_get_strk(dict, "dog");

	// verify correct error
	ck_assert(val_get_missing == NULL);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_add_and_copy)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_add_strk(&dict, "I have cats", &val);

	// copy value with key
	int val_get;
	whisker_dict_copy_strk(dict, "I have cats", &val_get);

	// verify gotten value is correct
	ck_assert_int_eq(val, val_get);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_get_index)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_add_strk(&dict, "I have cats", &val);

	// add another value with key
	int val2 = 8;
	whisker_dict_add_strk(&dict, "I have cats still", &val2);

	// get indexes in the dict array for the keys
	size_t* val_index;
	whisker_dict_get_index_strk(dict, "I have cats", &val_index);
	size_t* val2_index;
	whisker_dict_get_index_strk(dict, "I have cats still", &val2_index);

	// verify gotten indexes
	ck_assert_int_eq(0, *val_index);
	ck_assert_int_eq(1, *val2_index);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_clear)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_add_strk(&dict, "I have cats", &val);

	// add another value with key
	int val2 = 8;
	whisker_dict_add_strk(&dict, "I have cats still", &val2);

	// verify array size
	ck_assert_int_eq(2, whisker_arr_header(dict)->length);

	// clear all keys and values
	whisker_dict_clear(&dict);

	// verify array size
	ck_assert_int_eq(0, whisker_arr_header(dict)->length);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_contains_key)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_add_strk(&dict, "I have cats", &val);

	// add another value with key
	int val2 = 8;
	whisker_dict_add_strk(&dict, "I have cats still", &val2);

	// check if contains key
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "I have cats"));
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "I have cats still"));
	ck_assert_int_eq(false, whisker_dict_contains_key_strk(dict, "I have dogs"));

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_contains_value)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_add_strk(&dict, "cat", &val);

	int val_missing = 8;
	ck_assert_int_eq(true, whisker_dict_contains_value(dict, &val));
	ck_assert_int_eq(false, whisker_dict_contains_value(dict, &val_missing));

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_as_array)
{
	int* array;
	whisker_dict_create(&array, int, 0);

	// add some values
	for (int i = 0; i < 10; ++i)
	{
		int value = 100 * i;
		char key[6];
		sprintf(key, "key_%d", i);
		whisker_dict_set_strk(&array, key, &value);
	}

	ck_assert_int_eq(10, whisker_arr_length(array));

	// use array functions on the dict to check values
	for (int i = 0; i < whisker_arr_length(array); ++i)
	{
		int check_value = 100 * i;

		ck_assert_int_eq(check_value, array[i]);
	}

	// use array functions on the dict to check keys
	// TODO: this gives a stack-use-after-scope crash, needs investigating why
	/* void** keys = whisker_dict_keys(array); */
    /*  */
	/* // loop using dict array length, since they are supposed to match */
	/* for (int i = 0; i < whisker_arr_length(array); ++i) */
	/* { */
	/* 	char check_key[6]; */
	/* 	sprintf(check_key, "key_%d", i); */
    /*  */
	/* 	ck_assert_str_eq(check_key, keys[i]); */
	/* } */

	whisker_dict_free(array);
}
END_TEST

START_TEST(test_whisker_dict_set_and_get)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_set_strk(&dict, "cat", &val);

	// change and set the value again
	val = 77;
	whisker_dict_set_strk(&dict, "cat", &val);

	// get value with key
	int* val_get = whisker_dict_get_strk(dict, "cat");

	// verify gotten value is correct
	ck_assert_int_eq(val, *val_get);

	// get value with missing key
	char* val_get_missing = whisker_dict_get_strk(dict, "dog");

	// verify correct error
	ck_assert(val_get_missing == NULL);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_set_and_remove)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	whisker_dict_set_strk(&dict, "cat", &val);

	// get value with key
	int* val_get = whisker_dict_get_strk(dict, "cat");

	// verify gotten value is correct
	ck_assert_int_eq(val, *val_get);

	// remove value
	whisker_dict_remove_strk(&dict, "cat");

	// verify key no longer exists
	ck_assert_int_eq(false, whisker_dict_contains_key_strk(dict, "cat"));

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_set_and_get_multiple)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// set some values over and over with same key
	// this tests proper removal when the dict is size 1
	for (int i = 0; i < 10; i++)
	{
		int value = 100 * i;
		char* key = "key 1";
		whisker_dict_set_strk(&dict, key, &value);
	}
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 1"));

	// verify length is 1
	ck_assert_int_eq(1, whisker_arr_length(dict));

	// verify value is last set value
	ck_assert_int_eq(900, dict[0]);

	// set some new values with a different key
	// this tests proper removal when dict is size 2 and removed value is at the
	// end of the dict
	for (int i = 0; i < 10; i++)
	{
		int value = 1000 * i;
		char* key2 = "key 2";
		whisker_dict_set_strk(&dict, key2, &value);
	}
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 2"));
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 1"));

	// verify length is 2
	ck_assert_int_eq(2, whisker_arr_length(dict));

	// verify value is last set value
	ck_assert_int_eq(900, dict[0]);
	ck_assert_int_eq(9000, dict[1]);

	// and again, set some new values with a different key
	// tests proper removal with size 3 (validates any other greater sizes)
	for (int i = 0; i < 10; i++)
	{
		int value = 10000 * i;
		char* key = "key 3";
		whisker_dict_set_strk(&dict, key, &value);
	}
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 3"));
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 2"));
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 1"));

	// verify length is 3
	ck_assert_int_eq(3, whisker_arr_length(dict));

	// verify value is last set value
	ck_assert_int_eq(900, dict[0]);
	ck_assert_int_eq(9000, dict[1]);
	ck_assert_int_eq(90000, dict[2]);

	// verify key indexes
	size_t* index_1;
	size_t* index_2;
	size_t* index_3;
	whisker_dict_get_index_strk(dict, "key 1", &index_1);
	whisker_dict_get_index_strk(dict, "key 2", &index_2);
	whisker_dict_get_index_strk(dict, "key 3", &index_3);

	ck_assert_int_eq(*index_1, 0);
	ck_assert_int_eq(*index_2, 1);
	ck_assert_int_eq(*index_3, 2);

	// test removal repacking by removing the first key
	whisker_dict_remove_strk(&dict, "key 1");
	ck_assert_int_eq(false, whisker_dict_contains_key_strk(dict, "key 1"));
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 3"));
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 2"));
	ck_assert_int_eq(2, whisker_arr_length(dict));

	// repacking should find the original value of 900 in index 0
	// then move the end value which is 90000 to index 0
	// the middle value should be the same
	ck_assert_int_eq(90000, dict[0]);
	ck_assert_int_eq(9000, dict[1]);

	whisker_dict_get_index_strk(dict, "key 3", &index_1);
	whisker_dict_get_index_strk(dict, "key 2", &index_2);

	ck_assert_int_eq(*index_1, 0);
	ck_assert_int_eq(*index_2, 1);

	// test removal repacking by removing the first key again
	whisker_dict_remove_strk(&dict, "key 3");
	ck_assert_int_eq(false, whisker_dict_contains_key_strk(dict, "key 1"));
	ck_assert_int_eq(false, whisker_dict_contains_key_strk(dict, "key 3"));
	ck_assert_int_eq(true, whisker_dict_contains_key_strk(dict, "key 2"));
	ck_assert_int_eq(1, whisker_arr_length(dict));

	whisker_dict_get_index_strk(dict, "key 2", &index_1);

	ck_assert_int_eq(*index_1, 0);

	// since it's already been repacked once, this time the original middle
	// value will the last value, moved to the first index
	ck_assert_int_eq(9000, dict[0]);

	// remove the final key
	whisker_dict_remove_strk(&dict, "key 2");
	ck_assert_int_eq(0, whisker_arr_length(dict));

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_set_non_char_key)
{
	int* dict;
	whisker_dict_create(&dict, int, 0);

	// add a value with key
	int val = 7;
	size_t key_val = 123;
	char* key = (char*) &(size_t){key_val};
	whisker_dict_set_strk(&dict, key, &val);

	// get value with key
	int* val_get = whisker_dict_get_strk(dict, key);

	// verify gotten value is correct
	ck_assert_int_eq(val, *val_get);

	// remove value
	whisker_dict_remove_strk(&dict, key);

	// verify key no longer exists
	ck_assert_int_eq(false, whisker_dict_contains_key_strk(dict, key));

	char key_char[2] = {(char)123, 0};
	ck_assert_str_eq(key_char, key);

	whisker_dict_free(dict);
}
END_TEST

START_TEST(test_whisker_dict_ordered)
{
	uint64_t *dict;
	whisker_dict_create(&dict, uint64_t, 0);

	// add some values with unordered keys
	uint64_t val = 7;
	size_t key_index = 0;
	whisker_dict_set_keyt(&dict, &key_index, size_t, &val);

	val = 43;
	key_index = 4;
	whisker_dict_set_keyt(&dict, &key_index, size_t, &val);

	val = 12;
	key_index = 2;
	whisker_dict_set_keyt(&dict, &key_index, size_t, &val);

	val = 65;
	key_index = 7;
	whisker_dict_set_keyt(&dict, &key_index, size_t, &val);

	// get one value with key
	uint64_t* val_get = whisker_dict_get_keyt(dict, &key_index, size_t);

	// verify gotten value is correct
	ck_assert_uint_eq(val, *val_get);

	// use array functions on the dict to check values
	uint64_t expected[] = {7, 43, 12, 65};
	for (int i = 0; i < whisker_arr_length(dict); ++i)
	{
		ck_assert_uint_eq(expected[i], dict[i]);
	}

	// use array functions on the dict to check keys
	uint64_t** keys = (uint64_t**)whisker_dict_keys(dict);
	/* for (int i = 0; i < whisker_arr_length(keys); i++) { */
    /* 	printf("key %d: %zu\n", i, *keys[i]); */
    /* 	printf("value %d: %zu\n", i, dict[i]); */
	/* } */

	// loop using dict array length, since they are supposed to match
	uint64_t expected_keys[] = {0, 4, 2, 7};
	for (int i = 0; i < whisker_arr_length(dict); ++i)
	{
		ck_assert_uint_eq(expected_keys[i], *keys[i]);
	}

	whisker_dict_order_by_key((void**)&dict);

	/* for (int i = 0; i < whisker_arr_length(keys); i++) { */
    /* 	printf("key ordered %d: %zu\n", i, *keys[i]); */
    /* 	printf("value %d: %zu\n", i, dict[i]); */
	/* } */

	// use array functions on the dict to check values
	uint64_t expected_ordered[] = {7, 12, 43, 65};
	for (int i = 0; i < whisker_arr_length(dict); ++i)
	{
		ck_assert_uint_eq(expected_ordered[i], dict[i]);
	}

	// loop using dict array length, since they are supposed to match
	uint64_t expected_keys_ordered[] = {0, 2, 4, 7};
	for (int i = 0; i < whisker_arr_length(dict); ++i)
	{
		ck_assert_uint_eq(expected_keys_ordered[i], *keys[i]);
	}

	// verify keys point to new values
	for (int i = 0; i < whisker_arr_length(dict); ++i)
	{
		uint64_t *val_ordered_key = whisker_dict_get_keyt(dict, &expected_keys_ordered[i], size_t);
		ck_assert_uint_eq(*val_ordered_key, expected_ordered[i]);
	}

	whisker_dict_free(dict);
}
END_TEST

Suite* whisker_dict_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_dict");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_dict_create);
	tcase_add_test(tc_core, test_whisker_dict_add_and_get);
	tcase_add_test(tc_core, test_whisker_dict_add_and_get_string);
	tcase_add_test(tc_core, test_whisker_dict_add_and_copy);
	tcase_add_test(tc_core, test_whisker_dict_get_index);
	tcase_add_test(tc_core, test_whisker_dict_clear);
	tcase_add_test(tc_core, test_whisker_dict_contains_key);
	tcase_add_test(tc_core, test_whisker_dict_contains_value);
	tcase_add_test(tc_core, test_whisker_dict_as_array);
	tcase_add_test(tc_core, test_whisker_dict_set_and_get);
	tcase_add_test(tc_core, test_whisker_dict_set_and_remove);
	tcase_add_test(tc_core, test_whisker_dict_set_and_get_multiple);
	/* tcase_add_test(tc_core, test_whisker_dict_set_non_char_key); */
	tcase_add_test(tc_core, test_whisker_dict_ordered);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_dict_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
