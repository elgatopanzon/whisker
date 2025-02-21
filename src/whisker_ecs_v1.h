/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_v1
 * @created     : Tuesday Feb 11, 2025 13:33:11 CST
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef ECS_V1_H
#define ECS_V1_H

/*********
*  ECS  *
*********/
#ifndef ENTITY_MAX
#define ENTITY_MAX 128
#endif /* ifndef ENTITY_MAX */
#ifndef COMPONENT_MAX
#define COMPONENT_MAX 32
#endif /* ifndef ENTITY_MAX */
#ifndef SYSTEM_MAX
#define SYSTEM_MAX 32
#endif /* ifndef ENTITY_MAX */
extern size_t entity[ENTITY_MAX];
extern size_t entity_recycled[ENTITY_MAX];
extern size_t component_entity[COMPONENT_MAX][ENTITY_MAX];
// extern void* component_array[COMPONENT_MAX];
extern void (*system_array[SYSTEM_MAX])(float);

// ecs functions
void init_ecs();
void deinit_ecs();

// entity functions
void add_entity(size_t e);
void recycle_entity(size_t e);
void remove_entity(size_t e);
size_t get_recycled_entity();
size_t create_entity();

// component functions
bool add_component_entity(size_t component_id, size_t entity_id);
bool remove_component_entity(size_t component_id, size_t entity_id);
bool has_component_entity(size_t component_id, size_t entity_id);

// component array functions
void init_component_array(size_t component_id, size_t component_size);
void set_component(size_t component_id, size_t component_size, size_t entity, void* component_value);
void remove_component(size_t component_id, size_t component_size, size_t entity);
void* get_component(size_t component_id, size_t component_size, size_t entity);
size_t set_component_entities(size_t component_id, size_t entity_list[]);

#define GET_COMPONENT(component_id, type, entity) \
    ((type*)get_component(component_id, sizeof(type), entity))
#define REMOVE_COMPONENT(component_id, type, entity) \
    remove_component(component_id, sizeof(type), entity)
#define SET_COMPONENT(component_id, type, entity, value) \
	set_component(component_id, sizeof(type), entity, value)

// system functions
void register_system(void (*system_ptr)(float));
void deregister_system(void (*system_ptr)(float));
void update_systems(float delta_time);

#endif /* end of include guard ECS_V1_H */
