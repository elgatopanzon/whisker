/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:50 CST
 */

#include "whisker_std.h"
#include "whisker_block_array.h"
#include "whisker_dict.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_archetype.h"
#include "whisker_ecs_component.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_SYSTEM_H
#define WHISKER_ECS_SYSTEM_H

typedef struct whisker_ecs_system whisker_ecs_system;
typedef struct whisker_ecs_system_update
{
	whisker_ecs_entities *entities;
	whisker_ecs_components *components;
	whisker_ecs_system *system;
	whisker_ecs_entity_id entity_id;
	void *entity_components;
} whisker_ecs_system_update;

// component acting as a system container
struct whisker_ecs_system
{
	whisker_ecs_entity_id entity_id;
	void (*system_ptr)(struct whisker_ecs_system_update);
	int8_t thread_id;
	double last_update;
	double delta_time;

	// denotes which components it reads/writes
	whisker_ecs_entity_id *read_archetype;
	whisker_ecs_entity_id *write_archetype;

	whisker_ecs_components *read_components;
	whisker_ecs_components *write_components;
	int *component_name_index;

	whisker_ecs_components *components;

	char *read_component_names;
	char *write_component_names;
};

typedef struct whisker_ecs_systems
{
	whisker_ecs_system *systems;	
} whisker_ecs_systems;

// system management functions
E_WHISKER_ECS_SYS whisker_ecs_s_create_systems(whisker_ecs_systems **systems);
void whisker_ecs_s_free_systems(whisker_ecs_systems *systems);

// system operation functions
E_WHISKER_ECS_SYS whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system system);
E_WHISKER_ECS_SYS whisker_ecs_s_set_archetype_components(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system *system);
void whisker_ecs_s_free_system(whisker_ecs_system *system);
E_WHISKER_ECS_SYS whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time);
E_WHISKER_ECS_SYS whisker_ecs_s_update_system(whisker_ecs_system *system, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id entity_id);

// system component functions
void *whisker_ecs_s_get_read_component_by_index(whisker_ecs_system *system, size_t index, size_t size, whisker_ecs_entity_id entity_id);
void *whisker_ecs_s_get_write_component_by_index(whisker_ecs_system *system, size_t index, size_t size, whisker_ecs_entity_id entity_id);
void *whisker_ecs_s_get_read_component(whisker_ecs_system *system, char* component_name, size_t size, whisker_ecs_entity_id entity_id);
void *whisker_ecs_s_get_write_component(whisker_ecs_system *system, char* component_name, size_t size, whisker_ecs_entity_id entity_id);
void *whisker_ecs_s_get_component(whisker_ecs_system *system, whisker_ecs_components *archetype_components, whisker_ecs_entity_id *archetype, size_t index, size_t size, whisker_ecs_entity_id entity_id);
int whisker_ecs_s_get_component_name_index(whisker_ecs_system *system, char* component_names, char* component_name);

#endif /* WHISKER_ECS_SYSTEM_H */

