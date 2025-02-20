/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:56 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"

#include "whisker_ecs_system.h"

E_WHISKER_ECS_SYS whisker_ecs_s_create_systems(whisker_ecs_systems **systems)
{
	whisker_ecs_systems *s = calloc(1, sizeof(*s));
	if (s == NULL)
	{
		return E_WHISKER_ECS_SYS_MEM;
	}

	if (warr_create(whisker_ecs_system, 0, &s->systems) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_SYS_ARR;

	*systems = s;

	return E_WHISKER_ECS_SYS_OK;
}

void whisker_ecs_s_free_systems(whisker_ecs_systems *systems)
{
	for (int i = 0; i < warr_length(systems->systems); ++i)
	{
		whisker_ecs_s_free_system(&systems->systems[i]);
	}

	warr_free(systems->systems);

	free(systems);
}

E_WHISKER_ECS_SYS whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_system system)
{
	if (warr_push(&systems->systems, &system))
	{
		return E_WHISKER_ECS_SYS_ARR;
	}

	return E_WHISKER_ECS_SYS_OK;
}

void whisker_ecs_s_free_system(whisker_ecs_system *system)
{
	if (system->read_archetype != NULL)
	{
		warr_free(system->read_archetype);
	}
	if (system->write_archetype != NULL)
	{
		warr_free(system->write_archetype);
	}
	/* if (system->component_arrays != NULL) */
	/* { */
	/* 	warr_free(system->component_arrays); */
	/* } */
	/* if (system->components != NULL) */
	/* { */
	/* 	warr_free(system->components); */
	/* } */
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_components *components, double delta_time)
{
	// iterate over all systems, processing each entity and it's archetype for
	// matches, excluding updates for those which don't match
	for (int si = 0; si < warr_length(systems->systems); ++si)
	{
		whisker_ecs_system *system = &systems->systems[si];
		system->delta_time = delta_time;

		for (int ei = 0; ei < whisker_ecs_e_count(entities); ++ei)
		{
			whisker_ecs_entity *entity = &entities->entities[ei];

			if (whisker_ecs_a_match(system->write_archetype, entity->archetype))
			{
				whisker_ecs_s_update_system(system, entities, components, entity);				
			}
		}
	}

	return E_WHISKER_ECS_SYS_OK;
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_system(whisker_ecs_system *system, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity *entity)
{
	system->system_ptr((whisker_ecs_system_update) {
		entities = entities,
		components = components,
		system = system,
		entity = entity,
		});

	return E_WHISKER_ECS_SYS_OK;
}
