/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:50 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_SYSTEM_H
#define WHISKER_ECS_SYSTEM_H

typedef struct whisker_ecs_system
{
	whisker_ecs_entity_id entity_id;
	void (*system_ptr)(whisker_ecs_entity_id, double, struct whisker_ecs_system *);
	int8_t thread_id;
	double last_update;

	// denotes which components it reads/writes
	whisker_ecs_entity_id* read_archetype;
	whisker_ecs_entity_id* write_archetype;

	// component arrays and temp component pointer cache
	void *component_arrays[8];
	void *components[8];
} whisker_ecs_system;

typedef struct whisker_ecs_systems
{
	whisker_ecs_system *systems;	
} whisker_ecs_systems;

E_WHISKER_ECS_SYS whisker_ecs_s_create_systems(whisker_ecs_systems **systems);
void whisker_ecs_s_free_systems(whisker_ecs_systems *systems);

#endif /* WHISKER_ECS_SYSTEM_H */

