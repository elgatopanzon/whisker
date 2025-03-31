/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_world
 * @created     : Monday Mar 31, 2025 13:41:53 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_component.h"
#include "whisker_ecs_system.h"

#ifndef WHISKER_ECS_WORLD_H
#define WHISKER_ECS_WORLD_H

struct whisker_ecs_world
{
	whisker_ecs_entities *entities;
	whisker_ecs_components *components;
	whisker_ecs_systems *systems;
};

struct whisker_ecs_world *whisker_ecs_world_create();
struct whisker_ecs_world *whisker_ecs_world_create_and_init(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems);
void whisker_ecs_world_init(struct whisker_ecs_world *world, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems);

#endif /* WHISKER_ECS_WORLD_H */

