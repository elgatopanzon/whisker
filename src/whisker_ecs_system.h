/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:50 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_SYSTEM_H
#define WHISKER_ECS_SYSTEM_H

typedef struct whisker_ecs_system
{
	whisker_ecs_entity *entity;
	void (*system_ptr)(float);
	int8_t thread_id;
	double last_update;

	// denotes which components it reads/writes
	uint64_t read_archetype[8];
	uint64_t write_archetype[8];

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

