/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_entity_registry
 * @created     : Sunday Mar 01, 2026 17:08:32 CST
 * @description : entity registry supports named entities and recycling
 */

#include "whisker_std.h"
#include "whisker_arena.h"
#include "whisker_string_table.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_random.h"

#include <stdatomic.h>

#ifndef WHISKER_ENTITY_REGISTRY_H
#define WHISKER_ENTITY_REGISTRY_H

// tombstone, it will never be hit
#define W_ENTITY_INVALID UINT32_MAX

// block size used to re-allocate name maps and recycled stack
#ifndef WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE
#define WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE 4096
#endif

// check if an entity is valid
#define w_entity_is_valid(e) ((e) != WHISKER_ENTITY_INVALID)

// main entity ID is uint32
typedef uint32_t w_entity_id;

struct w_entity_registry
{
	// string table holds entity names
	struct w_string_table *name_table;

	// lookup tables: runtime entity id to name ID, and back
	w_array_declare(w_string_table_id, entity_to_name);
	w_array_declare(w_entity_id, name_to_entity);

	// recycle stack and ID counter (thread-safe via CAS)
	_Atomic w_entity_id next_id;
	w_array_declare(w_entity_id, recycled_stack);
};

// init entity registry
void w_entity_registry_init(struct w_entity_registry *registry, struct w_string_table *name_table);

// free entity registry name maps and recycled stack
void w_entity_registry_free(struct w_entity_registry *registry);

// request an entity
w_entity_id w_entity_request(struct w_entity_registry *registry);

// return an entity
void w_entity_return(struct w_entity_registry *registry, w_entity_id id);

// set entity name (convert anonymous entity to persistent)
void w_entity_set_name(struct w_entity_registry *registry, w_entity_id id, char *name);

// clear entity name (if it has one)
void w_entity_clear_name(struct w_entity_registry *registry, w_entity_id id);

// get entity name, null if not set
char *w_entity_get_name(struct w_entity_registry *registry, w_entity_id id);

// get entity ID from name
w_entity_id w_entity_lookup_by_name(struct w_entity_registry *registry, char *name);

// get alive entity count
#define w_entity_alive_count(r) (r->next_id - r->recycled_length)

// get recycled entity count
#define w_entity_recycled_count(r) (r->recycled_length)

// get total entity count
#define w_entity_total_count(r) (r->next_id)

#endif /* WHISKER_ENTITY_REGISTRY_H */

