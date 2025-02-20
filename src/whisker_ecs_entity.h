/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 17:53:46 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_ENTITY_H
#define WHISKER_ECS_ENTITY_H

typedef uint32_t whisker_ecs_entity_index;
typedef uint64_t whisker_ecs_entity_id_raw;

typedef struct whisker_ecs_entity_id
{
    union {
        whisker_ecs_entity_id_raw id;
        struct {
            whisker_ecs_entity_index index;
            whisker_ecs_entity_index version;
        };
        struct {
            whisker_ecs_entity_index entity_a;
            whisker_ecs_entity_index entity_b;
        };
        struct {
            uint16_t short1;
            uint16_t short2;
            uint16_t short3;
            uint16_t short4;
        };
    };
} whisker_ecs_entity_id;

typedef struct whisker_ecs_entity
{
    whisker_ecs_entity_id id;
    whisker_ecs_entity_id *archetype;
    char* name;
} whisker_ecs_entity;


typedef struct whisker_ecs_entities
{
	whisker_ecs_entity *entities;
	whisker_ecs_entity_index *dead_entities;
	whisker_ecs_entity_id *entity_names;
} whisker_ecs_entities;

// entity struct management functions
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_entities(whisker_ecs_entities **entities);
void whisker_ecs_e_free_entities(whisker_ecs_entities *entities);

// entity management functions
E_WHISKER_ECS_ENTITY whisker_ecs_e_create(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities, whisker_ecs_entity_index *entity_index);
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_new_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_set_name(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_named(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id *entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);

// utility functions
whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_e_id(whisker_ecs_entity_id_raw id);
whisker_ecs_entity* whisker_ecs_e_named(whisker_ecs_entities *entities, char* entity_name);
bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
size_t whisker_ecs_e_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_dead_count(whisker_ecs_entities *entities);
whisker_ecs_entity_id* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names);

// short macros and types
typedef whisker_ecs_entity_id wecs_e_id;
typedef whisker_ecs_entity_index wecs_e_idx;
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

