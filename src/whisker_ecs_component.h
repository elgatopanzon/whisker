/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 17:59:17 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_COMPONENT_H
#define WHISKER_ECS_COMPONENT_H

typedef struct whisker_ecs_components
{
	void **components;	
} whisker_ecs_components;

E_WHISKER_ECS_COMP whisker_ecs_c_create_components(whisker_ecs_components **components);
void whisker_ecs_c_free_components(whisker_ecs_components *components);

#endif /* WHISKER_ECS_COMPONENT_H */

