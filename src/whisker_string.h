/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_string
 * @created     : Thursday Feb 06, 2025 10:49:17 CST
 */

#include <stdbool.h>
#include <stdlib.h>
#include "whisker_array.h"

#ifndef WHISKER_STRING_H
#define WHISKER_STRING_H

// errors
typedef enum E_WHISKER_STR  
{
	E_WHISKER_STR_OK = 0,
	E_WHISKER_STR_UNKNOWN = 1,
	E_WHISKER_STR_MEM = 2,
	E_WHISKER_STR_ARR = 3,
} E_WHISKER_STR;
extern const char* E_WHISKER_STR_STR[];

// short macros
#define wstr whisker_str
#define wstr_join whisker_str_join
#define wstr_copy whisker_str_copy
#define wstr_free whisker_str_free
#define wstr_header whisker_str_header
#define wstr_length whisker_str_length
#define wstr_contains whisker_str_contains

// operation functions
E_WHISKER_STR whisker_str(char* str, char** w_str);
E_WHISKER_STR whisker_str_join(char* delimiter, char** w_str, ...);
E_WHISKER_STR whisker_str_copy(char* w_str_a, char** w_str_b);
void whisker_str_free(char* w_str);

// utility functions
whisker_array_header* whisker_str_header(char* w_str);
size_t whisker_str_length(char* w_str);
int whisker_str_contains(char* w_haystack, char* needle);


#endif /* end of include guard WHISKER_STRING_H */

