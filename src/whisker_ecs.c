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

	whisker_ecs_systems *s;
	if (whisker_ecs_s_create_systems(&s) != E_WHISKER_ECS_SYS_OK)
	{
		free(new);
		whisker_ecs_e_free_entities(e);
		whisker_ecs_c_free_components(c);

		return E_WHISKER_ECS_ARR;
	}

	new->entities = e;
	new->components = c;
	new->systems = s;

	*ecs = new;

	return E_WHISKER_ECS_OK;
}

void whisker_ecs_free(whisker_ecs *ecs)
{
	// free system components
	whisker_block_array *system_components = whisker_ecs_get_components(ecs, "whisker_ecs_system", sizeof(whisker_ecs_system));

	for (int i = 0; i < warr_length(system_components->blocks); ++i)
	{
		if (system_components->blocks[i] != NULL)
		{
			for (int ii = 0; ii < system_components->block_size; ++ii)
			{
				if (((whisker_ecs_system*)system_components->blocks[i])[ii].system_ptr != NULL)
				{
					warr_free(((whisker_ecs_system*)system_components->blocks[i])[ii].write_archetype);
				}
			}
		}
	}

	// free ecs state
	whisker_ecs_e_free_entities(ecs->entities);
	whisker_ecs_c_free_components(ecs->components);
	whisker_ecs_s_free_systems(ecs->systems);

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

						int match = 0;

						for (int sa = 0; sa < warr_length(system_archetype); ++sa)
						{
							/* printf("%d: checking system archetype id %d\n", entity.id.index, system_archetype[sa].index); */
							for (int ea = 0; ea < warr_length(entity_archetype); ++ea)
							{
								/* printf("%d: checking entity archetype id %d\n", entity.id.index, entity_archetype[ea].index); */
								if (system_archetype[sa].id == entity_archetype[ea].id)
								{
									match++;
									/* printf("archetype %d matches entity %d", system_archetype[sa].index, entity_archetype[ea].index); */
									break;
								}
							}
						}

						if (match >= warr_length(system_archetype))
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
		whisker_ecs_archetype_set(whisker_ecs_e(ecs->entities, entity_id), whisker_ecs_component_id(ecs, component_name));
	}
	else
	{
		whisker_ecs_archetype_remove(whisker_ecs_e(ecs->entities, entity_id), whisker_ecs_component_id(ecs, component_name));
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
	whisker_ecs_entity_id *entities;
	warr_create(whisker_ecs_entity_id, 0, &entities);

	char *name_temp;
	warr_create(char, 0, &name_temp);
	printf("archetype string %s\n", entity_names);

	for (int i = 0; i < strlen(entity_names); ++i)
	{
		printf("archetype string part %c\n", entity_names[i]);
		if (entity_names[i] == ',')
		{
			warr_push(&name_temp, &(char){'\0'});
			printf("archetype string %s\n", name_temp);
			whisker_ecs_entity_id id = whisker_ecs_component_id(ecs, name_temp);
			printf("archetype id %d\n", id.index);
			warr_push(&entities, &id);
			warr_free(name_temp);
			name_temp = NULL;
			continue;
		}

		if (name_temp == NULL)
		{
			warr_create(char, 0, &name_temp);
		}

		warr_push(&name_temp, &entity_names[i]);
	}

	if (name_temp != NULL)
	{
		warr_free(name_temp);
	}

	return entities;
}

E_WHISKER_ECS whisker_ecs_archetype_set(whisker_ecs_entity *entity, whisker_ecs_entity_id archetype_id)
{
	bool exists = false;
	for (int i = 0; i < warr_length(entity->archetype); ++i)
	{
		if (entity->archetype[i].id == archetype_id.id)
		{
			exists = true;
			break;
		}
	}

	if (!exists)
	{
		warr_push(&entity->archetype, &archetype_id);
	}

	return E_WHISKER_ECS_OK;
}
E_WHISKER_ECS whisker_ecs_archetype_remove(whisker_ecs_entity *entity, whisker_ecs_entity_id archetype_id)
{
	for (int i = 0; i < warr_length(entity->archetype); ++i)
	{
		if (entity->archetype[i].id == archetype_id.id)
		{
			whisker_arr_swap((void**) &entity->archetype, i, warr_length(entity->archetype) - 1);
			whisker_arr_pop(&entity->archetype, &archetype_id);
			break;
		}
	}

	return E_WHISKER_ECS_OK;
}

E_WHISKER_ECS whisker_ecs_set_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_archetype_set(whisker_ecs_e(ecs->entities, entity_id), whisker_ecs_component_id(ecs, component_name));
}

E_WHISKER_ECS whisker_ecs_remove_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_archetype_remove(whisker_ecs_e(ecs->entities, entity_id), whisker_ecs_component_id(ecs, component_name));
}

inline int whisker_ecs_get_archetype_index(whisker_ecs_entity_id *archetype, whisker_ecs_entity_id archetype_id)
{
	for (int i = 0; i < warr_length(archetype); ++i)
	{
		if (archetype[i].id == archetype_id.id)
		{
			return i;
		}
	}

	return -1;
}

bool whisker_ecs_has_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_get_archetype_index(whisker_ecs_e(ecs->entities, entity_id)->archetype, whisker_ecs_component_id(ecs, component_name));
}
