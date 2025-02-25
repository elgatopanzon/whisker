/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 17:53:56 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_dict.h"
#include "whisker_debug.h"
#include "whisker_string.h"

#include "whisker_ecs.h"

/*************************************
*  entities struct management  *
*************************************/
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_entities(whisker_ecs_entities **entities)
{
	whisker_ecs_entities *e;
	if (wmem_try_calloc_t(1, *e, &e) != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ECS_ENTITY_MEM;
	}

	// create arrays and dict
	if (warr_create(whisker_ecs_entity, 0, &e->entities) != E_WHISKER_ARR_OK)
	{
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (warr_create(whisker_ecs_entity_index, 0, &e->dead_entities) != E_WHISKER_ARR_OK)
	{
		warr_free(e->entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (warr_create(whisker_ecs_entity_deferred_action, 0, &e->deferred_actions) != E_WHISKER_ARR_OK)
	{
		warr_free(e->entities);
		warr_free(e->dead_entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (wdict_create(&e->entity_names, char*, 0) != E_WHISKER_DICT_OK)
	{
		warr_free(e->entities);
		warr_free(e->dead_entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_DICT;
	}

	if (warr_create(*e->archetype_changes, 0, &e->archetype_changes) != E_WHISKER_ARR_OK)
	{
		warr_free(e->entities);
		warr_free(e->dead_entities);
		wdict_free(e->entity_names);
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	// create empty entities up to the MIN
	for (int i = 0; i < WHISKER_ECS_ENTITY_MIN; ++i)
	{
		whisker_ecs_e_create(e);
	}

	*entities = e;

	return E_WHISKER_ECS_ENTITY_OK;
}

void whisker_ecs_e_free_entities(whisker_ecs_entities *entities)
{
	// free entity name wstrs
	for (int i = 0; i < warr_length(entities->entities); ++i)
	{
		if (entities->entities[i].name != NULL)
		{
			wstr_free(entities->entities[i].name);
		}

		warr_free(entities->entities[i].archetype);
	}

	warr_free(entities->entities);
	warr_free(entities->dead_entities);
	wdict_free(entities->entity_names);
	warr_free(entities->deferred_actions);
	warr_free(entities->archetype_changes);

	free(entities);
}

/***********************
*  entity management  *
***********************/

whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_(entities, &e);

	return e;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_create_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id)
{
	whisker_ecs_entity_id new_id;

	E_WHISKER_ECS_ENTITY err_create;
	if (whisker_ecs_e_dead_count(entities))
	{
		whisker_ecs_entity_index recycled_index;
		err_create = whisker_ecs_e_pop_recycled_(entities, &recycled_index);

		new_id = entities->entities[recycled_index].id;
		entities->entities[recycled_index].alive = true;
	}
	else
	{
		err_create = whisker_ecs_e_create_new_(entities, &new_id);
	}

	if (err_create != E_WHISKER_ECS_ENTITY_OK)
	{
		return err_create;
	}

	*entity_id = new_id;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities, whisker_ecs_entity_index *entity_index)
{
	if (warr_pop_front(&entities->dead_entities, entity_index) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_create_new_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id)
{
	size_t entity_count = whisker_ecs_e_count(entities);

	// TODO: check for max entities reached if there's a limit set later
	
	if (warr_increment_size(&entities->entities) != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	entities->entities[entity_count].id.index = entity_count;
	entities->entities[entity_count].alive = true;

	warr_create(whisker_ecs_entity_id, 0, &entities->entities[entity_count].archetype);

	*entity_id = entities->entities[entity_count].id;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_set_name(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id)
{
	E_WHISKER_DICT err = wdict_set(&entities->entity_names, name, &entity_id);
	if (err != E_WHISKER_DICT_OK)
	{
		return E_WHISKER_ECS_ENTITY_DICT;
	}

	// copy the name into the entities name
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	if (wstr(name, &e->name) != E_WHISKER_STR_OK)
	{
		return E_WHISKER_ECS_ENTITY_MEM;
	}

	return E_WHISKER_ECS_ENTITY_OK;
}

whisker_ecs_entity_id whisker_ecs_e_create_named(whisker_ecs_entities *entities, char *name)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_named_(entities, name, &e);

	return e;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_create_named_(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id *entity_id)
{
	whisker_ecs_entity *e = whisker_ecs_e_named(entities, name);
	if (e != NULL)
	{
		*entity_id = e->id;
		return E_WHISKER_ECS_ENTITY_OK;
	}

	// create new entity with name
	whisker_ecs_entity_id e_id;
	E_WHISKER_ECS_ENTITY create_err = whisker_ecs_e_create_(entities, &e_id);
	if (create_err != E_WHISKER_ECS_ENTITY_OK)
	{
		return create_err;
	}

	// set the name
	create_err = whisker_ecs_e_set_name(entities, name, e_id);
	if (create_err != E_WHISKER_ECS_ENTITY_OK)
	{
		return create_err;
	}

	*entity_id = e_id;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// increment version
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->id.version++;

	// clear name pointer
	if (e->name != NULL)
	{
		wdict_remove(&entities->entity_names, e->name);
		wstr_free(e->name);
		e->name = NULL;
	}

	// push to dead entities stack
	if (warr_push(&entities->dead_entities, &e->id.index) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);

	// move all archetypes into archetype changes
	for (int i = 0; i < warr_length(e->archetype); ++i)
	{
		whisker_ecs_e_set_archetype_changed(entities, e->id, e->archetype[i], false);
	}

	warr_resize(&e->archetype, 0);
	e->alive = false;

	if (whisker_ecs_e_recycle(entities, entity_id) != E_WHISKER_ECS_ENTITY_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_set_archetype_changed(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id, bool change_type)
{
	whisker_ecs_entity_archetype_change change = {
		.entity_id = entity_id,
		.archetype_id = archetype_id,
		.change_type = change_type
	};

	return (warr_push(&entities->archetype_changes, &change) == E_WHISKER_ARR_OK) ? E_WHISKER_ECS_ENTITY_OK : E_WHISKER_ECS_ENTITY_ARR;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_process_deferred(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_deferred_action action;
	while (warr_pop(&entities->deferred_actions, &action) == E_WHISKER_ARR_OK) 
	{
		switch (action.action) {
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE:
				entities->entities[action.id.index].alive = true;		
				break;
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY:
				whisker_ecs_e_destroy(entities, action.id);
				break;
		}
	}

	return E_WHISKER_ECS_ENTITY_OK;
}

/***********************
*  utility functions  *
***********************/

whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	if (entity_id.index + 1 > whisker_ecs_e_count(entities))
	{
		return NULL;
	}

	return &entities->entities[entity_id.index];
}

inline whisker_ecs_entity_id whisker_ecs_e_id(whisker_ecs_entity_id_raw id)
{
	return (whisker_ecs_entity_id){.id = id};
}

whisker_ecs_entity* whisker_ecs_e_named(whisker_ecs_entities *entities, char* entity_name)
{
	// lookup entity by name and return a match, or NULL
	whisker_ecs_entity_id *entity_id = wdict_get(entities->entity_names, entity_name);
	if (entity_id == NULL)
	{
		return NULL;
	}

	return whisker_ecs_e(entities, *entity_id);
}

bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return (whisker_ecs_e(entities, entity_id)->id.version == entity_id.version);
}

size_t whisker_ecs_e_count(whisker_ecs_entities *entities)
{
	return warr_length(entities->entities);
}

size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities)
{
	return whisker_ecs_e_count(entities) - whisker_ecs_e_dead_count(entities);
}

size_t whisker_ecs_e_dead_count(whisker_ecs_entities *entities)
{
	return warr_length(entities->dead_entities);
}

whisker_ecs_entity_id* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names_str)
{
	// entity list derived from string entity names
	whisker_ecs_entity_id *entities_new;
	warr_create(whisker_ecs_entity_id, 0, &entities_new);

	char* entity_names; wstr(entity_names_str, &entity_names);
	size_t names_length = wstr_length(entity_names);

	if (names_length == 0)
	{
		wstr_free(entity_names);
		return entities_new;
	}

	size_t search_index = 0;
	for (size_t i = 0; i < names_length + 1; ++i)
	{
		// if we reached the end of the string, or a separator
		// mutate to null terminator value
		bool mutated = false;
		if (entity_names[i] == ',') {
    		entity_names[i] = '\0';
    		mutated = true;
		}

		if (entity_names[i] == 0x0)
		{
			// create/get entity ID for name
			whisker_ecs_entity_id e;
			whisker_ecs_e_create_named_(entities, entity_names + search_index, &e);

			debug_printf("%zu-%zu: %s\n", search_index, i, entity_names + search_index);

			// add the entity id to the final list, and reset name array
			warr_push(&entities_new, &e);

			search_index = i + 1;
			if (mutated)
			{
    			entity_names[i] = ',';
			}
			continue;
		}
	}

	wstr_free(entity_names);

	whisker_ecs_e_sort_entity_array(entities_new);

	return entities_new;
}

int whisker_ecs_e_compare_entity_ids(const void *id_a, const void *id_b)
{
	return ((*(whisker_ecs_entity_id*)id_a).id - (*(whisker_ecs_entity_id*)id_b).id);
}

void whisker_ecs_e_sort_entity_array(whisker_ecs_entity_id *entities)
{
	qsort(entities, warr_length(entities), sizeof(whisker_ecs_entity_id), whisker_ecs_e_compare_entity_ids);
}
