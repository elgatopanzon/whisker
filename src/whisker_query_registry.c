/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_query_registry
 * @created     : Friday Mar 06, 2026 13:18:03 CST
 */

#include "whisker_std.h"
#include "whisker_hash_xxhash64.h"

#include "whisker_query_registry.h"

void w_query_registry_init(struct w_query_registry *registry, struct w_string_table *string_table, struct w_component_registry *component_registry, struct w_arena *arena)
{
	registry->string_table = string_table;
	registry->component_registry = component_registry;

	w_array_init_t(registry->queries, W_QUERY_REGISTRY_QUERIES_REALLOC_BLOCK_SIZE);

	w_hashmap_t_init(&registry->query_map, arena, 64, w_hashmap_hash_str, w_hashmap_eq_str);
}
void w_query_registry_free(struct w_query_registry *registry)
{
	// free all queries terms
	for (size_t i = 0; i < registry->queries_length; ++i)
	{
		struct w_query *q = &registry->queries[i];

		if (q->terms_length > 0)
		{
			free_null(q->terms);
		}
	}

	free_null(registry->queries);

	registry->queries_length = 0;

	// free hashmap
	w_hashmap_t_free(&registry->query_map);
}


static inline void w_query_registry_parse_query_string(struct w_query *query, struct w_string_table *string_table)
{
	// skip non-interned query
	if (query->raw_query == W_STRING_TABLE_INVALID_ID)
	{
		return;
	}

	char *raw_query = w_string_table_lookup(string_table, query->raw_query);

	// loop over the raw query string splitting on commas
	uint term_id = 0;
	while (raw_query && *raw_query)
	{
		char *query_term_comma = strchr(raw_query, ',');

		size_t term_string_len;
		if (query_term_comma)
		{
			// comma found: term is everything before comma
			term_string_len = query_term_comma - raw_query;
		}
		else
		{
			// no comma: last term is rest of string
			term_string_len = strlen(raw_query);
		}

		// skip empty terms (e.g. leading comma or double comma)
		if (term_string_len == 0)
		{
			if (query_term_comma)
			{
				raw_query = query_term_comma + 1;
				continue;
			}
			break;
		}

		// alloc and intern term string
		w_array_ensure_alloc_block_size(
			query->terms,
			term_id + 1,
			W_QUERY_REGISTRY_QUERY_TERMS_REALLOC_BLOCK_SIZE
		);

		query->terms[term_id].raw_term = w_string_table_intern_strn(string_table, raw_query, term_string_len);
		query->terms[term_id].component_id = W_ENTITY_INVALID;
		query->terms[term_id].component_name = W_STRING_TABLE_INVALID_ID;
		query->terms[term_id].access_type = W_QUERY_ACCESS_NONE;
		term_id++;

		// advance past the term
		if (query_term_comma)
		{
			// skip past comma
			raw_query = query_term_comma + 1;
			// skip whitespace after comma
			while (*raw_query == ' ')
			{
				raw_query++;
			}
		}
		else
		{
			// no more terms
			break;
		}
	}

	query->terms_length = term_id;

	if (term_id > 0)
	{
		query->query_parse_state = W_QUERY_PARSE_STATE_QUERY_PARSED;
	}
}
static inline void w_query_registry_parse_query_term_strings(struct w_query *query, struct w_string_table *string_table)
{
	int parsed_count = 0;
	for (size_t i = 0; i < query->terms_length; ++i)
	{
		struct w_query_term *term = &query->terms[i];

		// simple check of the component name string, this tells us that the
		// query term has been parsed with valid name strings interned
		if (term->component_name != W_STRING_TABLE_INVALID_ID)
		{
			parsed_count++;
			continue;
		}

		// attempt to parse the raw term string if its valid
		if (term->raw_term != W_STRING_TABLE_INVALID_ID)
		{
			char *raw_term = w_string_table_lookup(string_table, term->raw_term);

			// TODO: parse query strings into component name and access
    		const char *space = strchr(raw_term, ' ');

			// protect against invalid terms
    		if (!space || space == raw_term || *(space+1) == '\0') continue;

    		// extract lengths of each part
    		size_t type_len = space - raw_term;
    		size_t name_len = strlen(space + 1);

    		// parse extracted access type
    		if (strncmp(raw_term, "read", type_len) == 0)
    			term->access_type = W_QUERY_ACCESS_READ;
    		else if (strncmp(raw_term, "write", type_len) == 0)
    			term->access_type = W_QUERY_ACCESS_WRITE;
    		else if (strncmp(raw_term, "optional", type_len) == 0)
    			term->access_type = W_QUERY_ACCESS_OPTIONAL;
    		else
    			term->access_type = W_QUERY_ACCESS_NONE;

    		// intern component name
    		term->component_name = w_string_table_intern_strn(string_table, space + 1, name_len);

    		parsed_count++;
		}
	}

	if (parsed_count == (int)query->terms_length)
	{
		query->query_parse_state = W_QUERY_PARSE_STATE_TERMS_PARSED;
	}
}
static inline void w_query_registry_parse_query_term_components(struct w_query *query, struct w_string_table *string_table, struct w_component_registry *component_registry)
{
	int parsed_count = 0;
	for (size_t i = 0; i < query->terms_length; ++i)
	{
		struct w_query_term *term = &query->terms[i];

		// move on if component already parsed
		if (term->component_id != W_ENTITY_INVALID)
		{
			parsed_count++;
			continue;
		}

		// attempt to parse component name ID if both are valid
		if (term->component_id == W_ENTITY_INVALID && term->component_name != W_STRING_TABLE_INVALID_ID)
		{
			char *component_name = w_string_table_lookup(string_table, term->component_name);
			w_entity_id component_id = w_entity_lookup_by_name(component_registry->entities, component_name);

			if (component_id != W_ENTITY_INVALID)
			{
				term->component_id = component_id;
				parsed_count++;
			}
		}
	}

	if (parsed_count == (int)query->terms_length)
	{
		query->query_parse_state = W_QUERY_PARSE_STATE_COMPONENTS_PARSED;
	}
}

struct w_query *w_query_registry_get_query(struct w_query_registry *registry, char *query_string)
{
	struct w_query *query = NULL;

	// stage 0: check if query string exists in hashmap
	uint64_t *query_id;
	w_hashmap_t_get(&registry->query_map, query_string, query_id);

	// assign existing query to struct
	if (query_id)
		query = &registry->queries[*query_id];
	// bump and get new query struct
	else
	{
		w_array_ensure_alloc_block_size(
			registry->queries,
			registry->queries_length + 1,
			W_QUERY_REGISTRY_QUERIES_REALLOC_BLOCK_SIZE
		);

		uint64_t new_id = registry->queries_length++;
		query = &registry->queries[new_id];

		// init new query struct
		query->raw_query = w_string_table_intern_str(registry->string_table, query_string);
		query->query_parse_state = W_QUERY_PARSE_STATE_UNPARSED;

		w_array_init_t(query->terms, W_QUERY_REGISTRY_QUERY_TERMS_REALLOC_BLOCK_SIZE);
		query->bitset_cache_generation = 0;

		// add to hashmap for caching - use interned string as key
		char *interned_str = w_string_table_lookup(registry->string_table, query->raw_query);
		w_hashmap_t_set(&registry->query_map, interned_str, new_id);
	}

	// return query if fully parsed, skip parse stages
	if (query->query_parse_state == W_QUERY_PARSE_STATE_COMPONENTS_PARSED)
	{
		return query;
	}
	
	// stage 1: init query and parse query string into separate raw query terms
	if (query->query_parse_state == W_QUERY_PARSE_STATE_UNPARSED)
	{
		w_query_registry_parse_query_string(query, registry->string_table);
	}
	
	// stage 2: parse raw query terms and record component names
	if (query->query_parse_state == W_QUERY_PARSE_STATE_QUERY_PARSED)
	{
		w_query_registry_parse_query_term_strings(query, registry->string_table);
	}
	
	// stage 3: fetch component IDs from component strings
	if (query->query_parse_state == W_QUERY_PARSE_STATE_TERMS_PARSED)
	{
		w_query_registry_parse_query_term_components(query, registry->string_table, registry->component_registry);
	}

	return query;
}
