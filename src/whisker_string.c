/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_string
 * @created     : Thursday Feb 06, 2025 10:49:22 CST
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "whisker_array.h"
#include "whisker_string.h"

const char* E_WHISKER_STR_STR[] = {
	[E_WHISKER_STR_OK]="OK",
	[E_WHISKER_STR_UNKNOWN]="Unknown error",
	[E_WHISKER_STR_MEM]="Memory error during operation",
	[E_WHISKER_STR_ARR]="Array operation error",
};

// convert a string to a whisker headered string
E_WHISKER_STR whisker_str(char* str, char** w_str)
{
	size_t str_size = strlen(str) + 1;

	E_WHISKER_ARR err = whisker_arr_create(char, str_size, w_str);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_STR_ARR;
	}

	// copy string into w_str
	memcpy(*w_str, str, str_size);

	return E_WHISKER_STR_OK;
}

// join multiple strings together with a delimiter
E_WHISKER_STR whisker_str_join(char* delimiter, char** w_str, ...) {
	// count string args length and arg count
    va_list args;
    va_start(args, w_str);
    size_t joined_length = 0;
    size_t delimiter_length = strlen(delimiter);
    char* current_str;

    while ((current_str = va_arg(args, char*)) != NULL) {
        joined_length += strlen(current_str) + delimiter_length;
    }
    va_end(args);

    // create a whisker string matching the size
	E_WHISKER_ARR err = whisker_arr_create(char, joined_length, w_str);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_STR_ARR;
	}

    va_start(args, w_str);
    char* dest = *w_str;
    int first = 1;
    while ((current_str = va_arg(args, char*)) != NULL) {
        if (!first) {
            strncpy(dest, delimiter, delimiter_length);
            dest += delimiter_length;
        }
        size_t current_length = strlen(current_str);
        strncpy(dest, current_str, current_length + 1);
        dest += current_length;
        first = 0;
    }
    va_end(args);

    return E_WHISKER_STR_OK;
}

// copy A into B, allocating a new string
E_WHISKER_STR whisker_str_copy(char* w_str_a, char** w_str_b)
{
    // create a whisker string from existing string
    // NOTE: it's just a wrapper over whisker_str, really
    // it would even work with a normal string
	E_WHISKER_STR err = whisker_str(w_str_a, w_str_b);
	if (err != E_WHISKER_STR_OK)
	{
		return err;
	}

    return E_WHISKER_STR_OK;
}

// obtain the header from the string pointer
whisker_array_header* whisker_str_header(char* w_str)
{
	return whisker_arr_header(w_str);
}

// obtain string length from w string
size_t whisker_str_length(char* w_str)
{
	whisker_array_header* header = whisker_str_header(w_str);
	return header->length - 1; // -1 to cut off the \0 byte
}

// check if string (heystack) contains another string (needle)
int whisker_str_contains(char* w_haystack, char* needle)
{
    size_t haystack_length = whisker_str_length(w_haystack);
    size_t needle_length = strlen(needle);

    if (needle_length == 0) return -1;
    if (haystack_length < needle_length) return -1;

    for (size_t i = 0; i <= haystack_length - needle_length; i++) {
        size_t j;
        for (j = 0; j < needle_length; j++) {
            if (w_haystack[i + j] != needle[j]) {
                break;
            }
        }
        if (j == needle_length) {
            return i;
        }
    }
    return -1;
}

// free the string by obtaining the header
void whisker_str_free(char* w_str)
{
	warr_free(w_str);
}
