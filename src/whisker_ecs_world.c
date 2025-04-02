/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_world
 * @created     : Tuesday Apr 01, 2025 19:44:05 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"

// allocate an ECS world object
struct w_world *w_world_create()
{
	return w_mem_xcalloc_t(1, struct w_world);
}

// allocate and init an ECS world object
struct w_world *w_world_create_and_init(struct w_entities *entities, struct w_components *components, struct w_systems *systems)
{
	struct w_world *world = w_world_create();
	w_world_init(world, entities, components, systems);
	return world;
}

// init an ECS world object
void w_world_init(struct w_world *world, struct w_entities *entities, struct w_components *components, struct w_systems *systems)
{
	world->entities = entities;
	world->components = components;
	world->systems = systems;
}
