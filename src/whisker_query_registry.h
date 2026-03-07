/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_query_registry
 * @created     : Thursday Mar 05, 2026 20:12:33 CST
 * @description : query registry with parser and results cache
 */

#include "whisker_std.h"
#include "whisker_entity_registry.h"
#include "whisker_component_registry.h"
#include "whisker_sparse_bitset.h"
#include "whisker_hashmap.h"
#include "whisker_string_table.h"

#ifndef WHISKER_QUERY_REGISTRY_H
#define WHISKER_QUERY_REGISTRY_H

#ifndef W_QUERY_REGISTRY_QUERIES_REALLOC_BLOCK_SIZE
#define W_QUERY_REGISTRY_QUERIES_REALLOC_BLOCK_SIZE 1024
#endif /* ifndef W_QUERY_REGISTRY_QUERIES_REALLOC_BLOCK_SIZE */

#ifndef W_QUERY_REGISTRY_QUERY_TERMS_REALLOC_BLOCK_SIZE
#define W_QUERY_REGISTRY_QUERY_TERMS_REALLOC_BLOCK_SIZE 1024
#endif /* ifndef W_QUERY_REGISTRY_QUERY_TERMS_REALLOC_BLOCK_SIZE */

#ifndef W_QUERY_REGISTRY_QUERY_SLICES_REALLOC_BLOCK_SIZE
#define W_QUERY_REGISTRY_QUERY_SLICES_REALLOC_BLOCK_SIZE 8196
#endif /* ifndef W_QUERY_REGISTRY_QUERY_SLICES_REALLOC_BLOCK_SIZE */

#ifndef W_QUERY_REGISTRY_ARCHETYPE_SLICES_MIN_SLICE
#define W_QUERY_REGISTRY_ARCHETYPE_SLICES_MIN_SLICE 16
#endif /* ifndef W_QUERY_REGISTRY_ARCHETYPE_SLICES_MIN_SLICE */

#ifndef W_QUERY_REGISTRY_ARCHETYPE_SLICES_MAX_SLICE
#define W_QUERY_REGISTRY_ARCHETYPE_SLICES_MAX_SLICE 1024
#endif /* ifndef W_QUERY_REGISTRY_ARCHETYPE_SLICES_MIN_SLICE */

// each query term has a specific access
enum W_QUERY_ACCESS
{
	W_QUERY_ACCESS_NONE,
	W_QUERY_ACCESS_READ,
	W_QUERY_ACCESS_WRITE,
	W_QUERY_ACCESS_OPTIONAL,
};

// query parse state
enum W_QUERY_PARSE_STATE
{
	W_QUERY_PARSE_STATE_UNPARSED,
	W_QUERY_PARSE_STATE_QUERY_PARSED,
	W_QUERY_PARSE_STATE_TERMS_PARSED,
	W_QUERY_PARSE_STATE_COMPONENTS_PARSED,
};

// query archetype slice is the start index and number of contigious entity IDs
struct w_query_archetype_slice 
{
	w_entity_id start_id;
	size_t slice_length;
};

// full queries are made up of query terms
// valid term examples, commands not part of the term string:
// "read component_a"
// "write component_a"
// "optional component_a"
// etc...

struct w_query_term 
{
	w_string_table_id raw_term;
	w_entity_id component_id;
	w_string_table_id component_name;
	enum W_QUERY_ACCESS access_type;
};

// valid query examples, each one is a term:
// "read component_a, read component_b, write component_c, optional component_d"
// "read component_a, read component_b"
// "write component_a, write component_b"
// "write component_a,write component_b" note: missing white space
// INVALID: "write component_a write component_b" note: missing comma
// etc...

// main query struct
struct w_query 
{
	// raw query string
	w_string_table_id raw_query;
	enum W_QUERY_PARSE_STATE query_parse_state;

	// parts making up the full query
	w_array_declare(struct w_query_term, terms);

	// cache of entity IDs passing the bitset intersect + array of bitsets
	struct w_sparse_bitset_intersect_cache bitset_cache;
	uint64_t bitset_cache_generation;

	// query archetype slice cache
	w_array_declare(struct w_query_archetype_slice, archetype_slices_dense);
	w_array_declare(struct w_query_archetype_slice, archetype_slices_sparse);
};

// declare a hashmap for the string query to ID map
w_hashmap_t_declare(const char*, uint64_t, w_query_map);

struct w_query_registry 
{
	// string table for storing copies of raw query strings, component names and raw query terms
	struct w_string_table *string_table;

	// component registry to get component IDs
	struct w_component_registry *component_registry;	

	// array of queries
	w_array_declare(struct w_query, queries);

	// query string hashmap to local query index
	struct w_query_map query_map;
};


// init query registry
void w_query_registry_init(struct w_query_registry *registry, struct w_string_table *string_table, struct w_component_registry *component_registry, struct w_arena *arena);
// free query registry and queries, and query map
void w_query_registry_free(struct w_query_registry *registry);


// pass in a query string, get a parsed query struct back
struct w_query *w_query_registry_get_query(struct w_query_registry *registry, char *query_string);

// rebuild the query cache, returns true if built false if no change
bool w_query_rebuild_cache(struct w_query_registry *registry, struct w_query *query);

#endif /* WHISKER_QUERY_REGISTRY_H */

