/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 17:53:46 CST
 */

#include <pthread.h>
#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_trie.h"
#include "whisker_ecs_types.h"

#ifndef WHISKER_ECS_ENTITY_H
#define WHISKER_ECS_ENTITY_H

#define WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE (8096 / sizeof(whisker_ecs_entity))
#define WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE (8096 / sizeof(whisker_ecs_entity_index))
#define WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE (8096 / sizeof(whisker_ecs_entity_deferred_action))

whisker_arr_declare_struct(whisker_ecs_entity_id, whisker_ecs_entity_id_array);

// entity struct management functions
whisker_ecs_entities *whisker_ecs_e_create_and_init_entities();
whisker_ecs_entities *whisker_ecs_e_create_entities();
void whisker_ecs_e_init_entities(whisker_ecs_entities *entities);
void whisker_ecs_e_free_entities(whisker_ecs_entities *entities);
void whisker_ecs_e_free_entities_all(whisker_ecs_entities *entities);

// entity management functions
whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_e_create_(whisker_ecs_entities *entities);
whisker_ecs_entity_index whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities);
whisker_ecs_entity_id  whisker_ecs_e_create_new_(whisker_ecs_entities *entities);
void whisker_ecs_e_set_name(whisker_ecs_entities *entities, char *name, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_e_create_named(whisker_ecs_entities *entities, char *name);
whisker_ecs_entity_id whisker_ecs_e_create_named_(whisker_ecs_entities *entities, char *name);
void whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_e_add_deffered_action(whisker_ecs_entities *entities, whisker_ecs_entity_deferred_action action);
void whisker_ecs_e_process_deferred(whisker_ecs_entities *entities);

// utility functions
whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_e_id(whisker_ecs_entity_id_raw id);
whisker_ecs_entity* whisker_ecs_e_named(whisker_ecs_entities *entities, char* entity_name);
bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
size_t whisker_ecs_e_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_destroyed_count(whisker_ecs_entities *entities);
struct whisker_ecs_entity_id_array* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names);
int whisker_ecs_e_compare_entity_ids_(const void *id_a, const void *id_b);
void whisker_ecs_e_sort_entity_array(whisker_ecs_entity_id *entities, size_t length);

// short macros and types
typedef whisker_ecs_entity_id wecs_id;
typedef whisker_ecs_entity_index wecs_idx;
typedef whisker_ecs_entity wecs_entity;
typedef whisker_ecs_entities wecs_entities;

#define wecs_e_create_entities whisker_ecs_e_create_entities
#define wecs_e_free_entities whisker_ecs_e_free_entities
#define wecs_e_create whisker_ecs_e_create
#define wecs_e_set_name whisker_ecs_e_set_name
#define wecs_e_create_named whisker_ecs_e_create_named
#define wecs_e_recycle whisker_ecs_e_recycle
#define wecs_e_destroy whisker_ecs_e_destroy
#define wecs_e whisker_ecs_e
#define wecs_e_named whisker_ecs_e_named
#define wecs_e_is_alive whisker_ecs_e_is_alive
#define wecs_e_count whisker_ecs_e_count
#define wecs_e_alive_count whisker_ecs_e_alive_count
#define wecs_e_dead_count whisker_ecs_e_dead_count

#endif /* WHISKER_ECS_ENTITY_H */

