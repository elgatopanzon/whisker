/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 17:59:26 CST
 */

#include "whisker_std.h"
#include "whisker_dict.h"

#include "whisker_ecs.h"

E_WHISKER_ECS_COMP whisker_ecs_c_create_components(whisker_ecs_components **components)
{
	whisker_ecs_components *c = calloc(1, sizeof(*c));
	if (c == NULL)
	{
		return E_WHISKER_ECS_COMP_MEM;
	}

	// create dict
	if (wdict_create(&c->components, void*, 0) != E_WHISKER_DICT_OK)
	{
		free(c);
		return E_WHISKER_ECS_COMP_DICT;
	}

	*components = c;

	return E_WHISKER_ECS_COMP_OK;
}

void whisker_ecs_c_free_components(whisker_ecs_components *components)
{
	wdict_free(components->components);

	free(components);
}
