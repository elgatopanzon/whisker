/**
 * @author      : ElGatoPanzon
 * @file        : whisker_relationships
 * @created     : Friday Mar 13, 2026 17:44:31 CST
 * @description : Entity relationship tracking with pair-map and adjacency indices
 */

#ifndef WHISKER_RELATIONSHIPS_H
#define WHISKER_RELATIONSHIPS_H

#include "whisker.h"

/*****************************
*  block realloc sizes       *
*****************************/

#ifndef W_RELATIONSHIP_PAIR_MAP_BUCKET_COUNT
#define W_RELATIONSHIP_PAIR_MAP_BUCKET_COUNT 256
#endif

#ifndef W_RELATIONSHIP_ADJACENCY_BUCKET_COUNT
#define W_RELATIONSHIP_ADJACENCY_BUCKET_COUNT 256
#endif

#ifndef W_RELATIONSHIP_ENTRIES_REALLOC_BLOCK_SIZE
#define W_RELATIONSHIP_ENTRIES_REALLOC_BLOCK_SIZE 16
#endif

#ifndef W_RELATIONSHIP_ADJACENCY_REALLOC_BLOCK_SIZE
#define W_RELATIONSHIP_ADJACENCY_REALLOC_BLOCK_SIZE 16
#endif


/*****************************
*  pair encoding             *
*****************************/

// pack two entity IDs into a single uint64_t (min in high bits, max in low)
static inline uint64_t w_relationship_pack_pair(w_entity_id a, w_entity_id b)
{
	w_entity_id lo = (a < b) ? a : b;
	w_entity_id hi = (a < b) ? b : a;
	return ((uint64_t)lo << 32) | (uint64_t)hi;
}

// unpack a packed pair into its two entity IDs
static inline void w_relationship_unpack_pair(uint64_t packed, w_entity_id *lo, w_entity_id *hi)
{
	*lo = (w_entity_id)(packed >> 32);
	*hi = (w_entity_id)(packed & 0xFFFFFFFF);
}


/*****************************
*  data structures           *
*****************************/

// a single relationship entry: owner -> target via component
struct w_relationship_entry
{
	w_entity_id owner;
	w_entity_id target;
	w_entity_id component_id;
};

// list of relationship entries (value type for pair_map)
struct w_relationship_entry_list
{
	w_array_declare(struct w_relationship_entry, entries);
};

// list of adjacent entity IDs (value type for adjacency map)
struct w_relationship_adjacency_list
{
	w_array_declare(w_entity_id, entities);
};

// typed hashmap: uint64_t (packed pair) -> entry list
w_hashmap_t_declare(uint64_t, struct w_relationship_entry_list, w_relationship_pair_map);

// typed hashmap: w_entity_id -> adjacency list
w_hashmap_t_declare(w_entity_id, struct w_relationship_adjacency_list, w_relationship_adjacency_map);

// relationship registry holds pair_map and adjacency index
struct w_relationship_registry
{
	struct w_arena *arena;
	struct w_relationship_pair_map pair_map;
	struct w_relationship_adjacency_map adjacency;
};


/*****************************
*  registry API              *
*****************************/

// init the relationship registry with an arena
void w_relationship_registry_init(struct w_relationship_registry *reg, struct w_arena *arena);

// free all internal arrays and hashmap entries
void w_relationship_registry_free(struct w_relationship_registry *reg);

// add a relationship: owner -> target via component_id
void w_relationship_registry_add(struct w_relationship_registry *reg,
	w_entity_id owner, w_entity_id target, w_entity_id component_id);

// remove a specific relationship, returns true if found
bool w_relationship_registry_remove(struct w_relationship_registry *reg,
	w_entity_id owner, w_entity_id target, w_entity_id component_id);

// remove all relationships involving an entity (both as owner and target)
void w_relationship_registry_remove_entity(struct w_relationship_registry *reg, w_entity_id entity);

// get the entry list for a pair, NULL if no relationships between them
struct w_relationship_entry_list *w_relationship_registry_get_pair(
	struct w_relationship_registry *reg, w_entity_id a, w_entity_id b);

// get the adjacency list for an entity, NULL if no relationships
struct w_relationship_adjacency_list *w_relationship_registry_get_adjacent(
	struct w_relationship_registry *reg, w_entity_id entity);


/*****************************
*  module API (world-level)  *
*****************************/

// initialise relationship tracking on the world
void wm_relationships_init(struct w_ecs_world *world);

// cleanup relationship registry
void wm_relationships_free(struct w_ecs_world *world);

// add a relationship between entities in the world
void wm_relationships_add(struct w_ecs_world *world,
	w_entity_id owner, w_entity_id target, w_entity_id component_id);

// remove a specific relationship from the world, returns true if found
bool wm_relationships_remove(struct w_ecs_world *world,
	w_entity_id owner, w_entity_id target, w_entity_id component_id);

// get the entry list for a pair of entities
struct w_relationship_entry_list *wm_relationships_get_pair(
	struct w_ecs_world *world, w_entity_id a, w_entity_id b);

// get adjacency list for an entity
struct w_relationship_adjacency_list *wm_relationships_get_adjacent(
	struct w_ecs_world *world, w_entity_id entity);

// get the relationship registry singleton from the world
struct w_relationship_registry *wm_relationships_get_registry(struct w_ecs_world *world);


/*****************************
*  command buffer functions  *
*****************************/

// buffered command: remove all relationships for an entity
void wm_relationships_cmd_remove_entity_(void *world, void *payload);

#endif /* WHISKER_RELATIONSHIPS_H */
