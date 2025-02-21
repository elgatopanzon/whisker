/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_archetype
 * @created     : Tuesday Feb 18, 2025 10:51:45 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"

#include "whisker_ecs_archetype.h"

/*************************
*  archetype functions  *
*************************/
E_WHISKER_ECS_ARCH whisker_ecs_a_set(whisker_ecs_entity_id **archetype, whisker_ecs_entity_id archetype_id)
{
	int idx = whisker_ecs_a_has_id(*archetype, archetype_id);
	if (idx == -1)
	{
		warr_push(&*archetype, &archetype_id);
	}

	return E_WHISKER_ECS_ARCH_OK;
}

E_WHISKER_ECS_ARCH whisker_ecs_a_remove(whisker_ecs_entity_id **archetype, whisker_ecs_entity_id archetype_id)
{
	int idx = whisker_ecs_a_has_id(*archetype, archetype_id);
	if (idx > -1)
	{
		whisker_arr_swap((void**) &*archetype, idx, warr_length(*archetype) - 1);
		whisker_arr_pop(&*archetype, &archetype_id);
	}

	return E_WHISKER_ECS_ARCH_OK;
}

E_WHISKER_ECS_ARCH whisker_ecs_a_free(whisker_ecs_entity_id *archetype)
{
	warr_free(archetype);

	return E_WHISKER_ECS_ARCH_OK;
}


/************************
*  utility functions   *
************************/
inline int whisker_ecs_a_has_id(whisker_ecs_entity_id *archetype, whisker_ecs_entity_id archetype_id) {
    size_t len = warr_length(archetype);
    for (size_t i = 0; i < len; ++i) {
        if (memcmp(&archetype[i], &archetype_id, sizeof(whisker_ecs_entity_id)) == 0) {
            return i;
        }
    }
    return -1;
}

bool whisker_ecs_a_match(whisker_ecs_entity_id *archetype_a, whisker_ecs_entity_id *archetype_b) {
    size_t a_length = warr_length(archetype_a);
    for (size_t i = 0; i < a_length; ++i) {
        if (whisker_ecs_a_has_id(archetype_b, archetype_a[i]) == -1) {
            return false;
        }
    }
    return true;
}

whisker_ecs_entity_id* whisker_ecs_a_from_named_entities(whisker_ecs_entities *entities, char* entity_names)
{
	return whisker_ecs_e_from_named_entities(entities, entity_names);
}
