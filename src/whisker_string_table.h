/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_string_table
 * @created     : Sunday Mar 01, 2026 13:24:58 CST
 * @description : string interning with arena
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_arena.h"
#include "whisker_hashmap.h"

#ifndef WHISKER_STRING_TABLE_H
#define WHISKER_STRING_TABLE_H

// default string table entries buffer realloc block size
#ifndef WHISKER_STRING_TABLE_REALLOC_SIZE
#define WHISKER_STRING_TABLE_REALLOC_SIZE 4096
#endif /* ifndef WHISKER_STRING_TABLE_REALLOC_SIZE */

// default string table managed hashmap bucket size
#ifndef WHISKER_STRING_TABLE_BUCKETS_SIZE
#define WHISKER_STRING_TABLE_BUCKETS_SIZE 4096
#endif /* ifndef WHISKER_STRING_TABLE_BUCKETS_SIZE */

#define W_STRING_TABLE_INVALID_ID UINT32_MAX

// interned strings get stored in a string entry struct
// the struct holds the real string pointer and its length
struct w_string_table_entry
{
	char *string;
	size_t length;
};

// alias type for storing string table index
typedef uint32_t w_string_table_id;

// declare a hashmap for the string table reverse lookup
w_hashmap_t_declare(const char*, uint32_t, w_string_table_map);

// the string table struct holds an arena, and has self-managed buffer for
// entries and self-managed reverse string hashmap
struct w_string_table 
{
	struct w_arena *arena;
	w_array_declare(struct w_string_table_entry, entries);
	size_t entries_realloc_block_size;
	struct w_string_table_map reverse_map;
};

// init a w_string_table struct using the passed in arena
void w_string_table_init(struct w_string_table *table, struct w_arena *arena, size_t entries_realloc_block_size, size_t hashmap_bucket_count, w_hashmap_hash_fn string_hash_fn);

// free w_string_table's entries and hashmap
void w_string_table_free(struct w_string_table *table);

// intern a string into the string table, get back the ID
w_string_table_id w_string_table_intern_str(struct w_string_table *table, const char *string);
// intern a string into the string table with specified length, get back the ID
w_string_table_id w_string_table_intern_strn(struct w_string_table *table, const char *string, size_t string_length);

// lookup a string by ID, get the string pointer
char *w_string_table_lookup(struct w_string_table *table, w_string_table_id id);
// lookup a string by ID, get the complete string entry
struct w_string_table_entry *w_string_table_lookup_entry(struct w_string_table *table, w_string_table_id id);
// lookup a string ID by string, get the entry or NULL
struct w_string_table_entry *w_string_table_lookup_entry_str(struct w_string_table *table, const char *string);
// lookup an ID by string
w_string_table_id w_string_table_lookup_str(struct w_string_table *table, const char *string);

#endif /* WHISKER_STRING_TABLE_H */

