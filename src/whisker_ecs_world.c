/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_world
 * @created     : Monday Mar 31, 2025 13:43:12 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_world.h"

// allocate an ECS world object
struct whisker_ecs_world *whisker_ecs_world_create()
{
	return whisker_mem_xcalloc_t(1, struct whisker_ecs_world);
}

// allocate and init an ECS world object
struct whisker_ecs_world *whisker_ecs_world_create_and_init(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems)
{
	struct whisker_ecs_world *world = whisker_ecs_world_create();
	whisker_ecs_world_init(world, entities, components, systems);
	return world;
}

// init an ECS world object
void whisker_ecs_world_init(struct whisker_ecs_world *world, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems)
{
	world->entities = entities;
	world->components = components;
	world->systems = systems;
}
