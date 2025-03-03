/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_types
 * @created     : Sunday Mar 02, 2025 15:30:18 CST
 */

#include "whisker_std.h"
#include "whisker_sparse_set.h"
#include "whisker_ecs_types_base.h"
#include "generics/whisker_generic_array_whisker_ecs_entity.h"
#include "generics/whisker_generic_array_whisker_ecs_entity_id.h"
#include "generics/whisker_generic_array_whisker_ecs_entity_index.h"
#include "generics/whisker_generic_array_whisker_ecs_entity_deferred_action.h"
#include "generics/whisker_generic_array_void_.h"

#ifndef WHISKER_ECS_TYPES_H
#define WHISKER_ECS_TYPES_H


// the main entities struct
typedef struct whisker_ecs_entities
{
	// current list of entities used by the system
	whisker_arr_whisker_ecs_entity *entities;

	// stack of destroyed entities, used when recycling
	whisker_arr_whisker_ecs_entity_index *destroyed_entities;

	// dictionary of entity names mapping to indexes
	whisker_ecs_entity_id *entity_names;

	// stack of deferred actions to process
	whisker_arr_whisker_ecs_entity_deferred_action *deferred_actions;
} whisker_ecs_entities;


///////////////////////
//  component types  //
///////////////////////

typedef struct whisker_ecs_components
{
	whisker_sparse_set **components;
} whisker_ecs_components;


////////////////////
//  system types  //
////////////////////

typedef struct whisker_ecs_iterator
{
	// the master index points to the sparse set we're currently iterating
	size_t master_index;

	// current cursor position in the master iterator
	size_t cursor;
	size_t count;
	whisker_ecs_entity_id entity_id;

	// array of component name IDs
	whisker_arr_whisker_ecs_entity_id *component_ids;
	whisker_arr_void_ *component_arrays;

	// read/write arrays for the components of the current iteration step
	whisker_arr_void_ *read;
	whisker_arr_void_ *write;

} whisker_ecs_iterator;


// component acting as a system container
struct whisker_ecs_system
{
	whisker_ecs_entity_id entity_id;
	void (*system_ptr)(struct whisker_ecs_system*);
	int8_t thread_id;
	double last_update;
	double delta_time;

	whisker_ecs_components *components_cache;
	whisker_ecs_entity_id *components_cache_entities;
	int *component_name_index;

	whisker_ecs_components *components;
	whisker_ecs_entities *entities;
	whisker_sparse_set *iterators;
};

#endif /* WHISKER_ECS_TYPES_H */

