/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:29 CST
 */

#include "whisker_std.h"

#include "whisker_array.h"
#include "whisker_block_array.h"
#include "whisker_ecs.h"

E_WHISKER_ECS whisker_ecs_create(whisker_ecs **ecs)
{
	whisker_ecs *new = calloc(1, sizeof(*new));
	if (new == NULL)
	{
		return E_WHISKER_ECS_MEM;
	}

	whisker_ecs_entities *e;
	if (whisker_ecs_e_create_entities(&e) != E_WHISKER_ECS_ENTITY_OK)
	{
		free(new);

		return E_WHISKER_ECS_ARR;
	}

	whisker_ecs_components *c;
	if (whisker_ecs_c_create_components(&c) != E_WHISKER_ECS_COMP_OK)
	{
		free(new);
		whisker_ecs_e_free_entities(e);

		return E_WHISKER_ECS_ARR;
	}

	new->entities = e;
	new->components = c;

	*ecs = new;

	return E_WHISKER_ECS_OK;
}

void whisker_ecs_free(whisker_ecs *ecs)
{
	// free system components
	whisker_block_array *system_components = whisker_ecs_get_components(ecs, "whisker_ecs_system", sizeof(whisker_ecs_system));

	whisker_ecs_s_free_components(system_components);

	// free ecs state
	whisker_ecs_e_free_entities(ecs->entities);
	whisker_ecs_c_free_components(ecs->components);

	free(ecs);
}


/**********************
*  system functions  *
**********************/
E_WHISKER_ECS whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(whisker_ecs_entity_id, double, struct whisker_ecs_system *), char *component_archetype_names)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create(ecs->entities, &e);

	whisker_ecs_c_set_component(ecs->components, whisker_ecs_component_id(ecs, "whisker_ecs_system"), sizeof(whisker_ecs_system), e, &(whisker_ecs_system) {
		.entity_id = e,
		.system_ptr = system_ptr,
		.write_archetype = whisker_ecs_archetype_from_named_entities(ecs, component_archetype_names),
	});

	return E_WHISKER_ECS_OK;
}

E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time)
{
	whisker_block_array *system_components = whisker_ecs_get_components(ecs, "whisker_ecs_system", sizeof(whisker_ecs_system));

	for (int ei = 0; ei < whisker_ecs_e_count(ecs->entities); ++ei)
	{
		whisker_ecs_entity entity = ecs->entities->entities[ei];

		for (int i = 0; i < warr_length(system_components->blocks); ++i)
		{
			if (system_components->blocks[i] != NULL)
			{
				for (int ii = 0; ii < system_components->block_size; ++ii)
				{
					if (((whisker_ecs_system*)system_components->blocks[i])[ii].system_ptr != NULL)
					{
						whisker_ecs_entity_id *system_archetype = ((whisker_ecs_system*)system_components->blocks[i])[ii].write_archetype;
						whisker_ecs_entity_id *entity_archetype = ecs->entities->entities[ei].archetype;


						if (whisker_ecs_a_match(system_archetype, entity_archetype))
						{
							((whisker_ecs_system*)system_components->blocks[i])[ii].system_ptr(entity.id, delta_time, &((whisker_ecs_system*)system_components->blocks[i])[ii]);
						}
					}
				}
			}
		}
	}

	return E_WHISKER_ECS_OK;
}


/*************************
*  component functions  *
*************************/
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs *ecs, char* component_name)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_named(ecs->entities, component_name, &e);

	return e;
}

whisker_block_array* whisker_ecs_get_components(whisker_ecs *ecs, char* component_name, size_t component_size)
{
	whisker_block_array *components;
	whisker_ecs_c_get_component_array(ecs->components, whisker_ecs_component_id(ecs, component_name), component_size, (void**)&components);

	return components;
}

whisker_block_array* whisker_ecs_get_component(whisker_ecs *ecs, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id)
{
	whisker_block_array *components = whisker_ecs_get_components(ecs, component_name, component_size);

	return wbarr_get(components, entity_id.index);
}

E_WHISKER_ECS whisker_ecs_set_component(whisker_ecs *ecs, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id, void* value)
{
	whisker_block_array *components = whisker_ecs_get_components(ecs, component_name, component_size);

	if (value != NULL)
	{
		wbarr_set(components, entity_id.index, value);
		whisker_ecs_archetype_set(ecs, entity_id, whisker_ecs_component_id(ecs, component_name));
	}
	else
	{
		whisker_ecs_archetype_remove(ecs, entity_id, whisker_ecs_component_id(ecs, component_name));
	}

	return E_WHISKER_ECS_OK;
}

E_WHISKER_ECS whisker_ecs_remove_component(whisker_ecs *ecs, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_set_component(ecs, component_name, component_size, entity_id, NULL);
}


/*************************
*  archetype functions  *
*************************/
whisker_ecs_entity_id* whisker_ecs_archetype_from_named_entities(whisker_ecs *ecs, char* entity_names)
{
	return whisker_ecs_e_from_named_entities(ecs->entities, entity_names);
}

E_WHISKER_ECS whisker_ecs_archetype_set(whisker_ecs *ecs, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(ecs->entities, entity_id);

	if (whisker_ecs_a_set(&e->archetype, archetype_id) != E_WHISKER_ECS_ARCH_OK)
	{
		// TODO: figure out what can fail during this call
		return E_WHISKER_ECS_UNKNOWN;
	}

	return E_WHISKER_ECS_OK;
}
E_WHISKER_ECS whisker_ecs_archetype_remove(whisker_ecs *ecs, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(ecs->entities, entity_id);

	if (whisker_ecs_a_remove(&e->archetype, archetype_id) != E_WHISKER_ECS_ARCH_OK)
	{
		// TODO: figure out what can fail during this call
		return E_WHISKER_ECS_UNKNOWN;
	}

	return E_WHISKER_ECS_OK;
}

E_WHISKER_ECS whisker_ecs_set_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(ecs, component_name);
	return whisker_ecs_archetype_set(ecs, entity_id, component_id);
}

E_WHISKER_ECS whisker_ecs_remove_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(ecs, component_name);
	return whisker_ecs_archetype_remove(ecs, entity_id, component_id);
}

bool whisker_ecs_has_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(ecs, component_name);
	whisker_ecs_entity_id *archetype = whisker_ecs_e(ecs->entities, entity_id)->archetype;

	return whisker_ecs_a_has_id(archetype, component_id);
}
