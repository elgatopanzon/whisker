/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_string_table
 * @created     : Sunday Mar 01, 2026 14:13:32 CST
 */

#include "whisker_std.h"

#include "whisker_string_table.h"


void w_string_table_init(struct w_string_table *table, struct w_arena *arena, size_t entries_realloc_block_size, size_t hashmap_bucket_count, w_hashmap_hash_fn string_hash_fn)
{
	(void)string_hash_fn; // unused, we use w_hashmap_hash_str for string keys
	table->arena = arena;
	table->entries_realloc_block_size = entries_realloc_block_size;

	// init array
	w_array_init_t(table->entries, entries_realloc_block_size);

	// init hashmap with string-specific hash/equality functions
	w_hashmap_t_init(&(table->reverse_map), arena, hashmap_bucket_count, w_hashmap_hash_str, w_hashmap_eq_str);
}

void w_string_table_free(struct w_string_table *table)
{
	// note: we don't free the strings, they are all in the arena
	//
	free_null(table->entries);
	w_hashmap_t_free(&(table->reverse_map));
}

w_string_table_id w_string_table_intern_str(struct w_string_table *table, const char *string)
{
	return w_string_table_intern_strn(table, string, strlen(string));
}

w_string_table_id w_string_table_intern_strn(struct w_string_table *table, const char *string, size_t string_length)
{
	// intern the string first (need null-terminated copy for hashmap lookup)
	w_array_ensure_alloc_block_size(table->entries, table->entries_length + 1, table->entries_realloc_block_size);

	// create entry with null-terminated copy
	struct w_string_table_entry *entry = &table->entries[table->entries_length];
	entry->string = w_arena_malloc(table->arena, string_length + 1);
	memcpy(entry->string, string, string_length);
	entry->string[string_length] = '\0';
	entry->length = string_length;

	// check if string already exists in reverse hashmap (uses entry->string which is null-terminated)
	uint32_t *existing_id;
	w_hashmap_t_get(&table->reverse_map, entry->string, existing_id);

	if (existing_id)
	{
		// already exists, arena memory for entry->string is "leaked" but will be freed with arena
		return (w_string_table_id)*existing_id;
	}

	// add to reverse map
	w_hashmap_t_set(&table->reverse_map, entry->string, (uint32_t)table->entries_length);

	return (w_string_table_id) table->entries_length++;
}

char *w_string_table_lookup(struct w_string_table *table, w_string_table_id id)
{
	struct w_string_table_entry *entry = w_string_table_lookup_entry(table, id);
	return (entry) ? entry->string : NULL;
}

struct w_string_table_entry *w_string_table_lookup_entry(struct w_string_table *table, w_string_table_id id)
{
	// early out
	if (id >= table->entries_length) { return NULL; }

	return &table->entries[id];
}

struct w_string_table_entry *w_string_table_lookup_entry_str(struct w_string_table *table, const char *string)
{
	w_string_table_id *id;
	w_hashmap_t_get(&table->reverse_map, string, id);

	return (id) ? w_string_table_lookup_entry(table, *id) : NULL;
}
