/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 17:53:46 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_err.h"

// reserve entities for system use by setting a minimum
// the default is only to reserve entity 0 for programatic reasons
#ifndef WHISKER_ECS_ENTITY_MIN
#define WHISKER_ECS_ENTITY_MIN 1
#endif

#ifndef WHISKER_ECS_ENTITY_H
#define WHISKER_ECS_ENTITY_H

// the main entity index is a uint32
typedef uint32_t whisker_ecs_entity_index;
// the raw entity ID is a uint64
typedef uint64_t whisker_ecs_entity_id_raw;

// main struct provides access to various forms of the unit64 ID
typedef struct whisker_ecs_entity_id
{
    union {
    	// the full raw uint64 ID
        whisker_ecs_entity_id_raw id;

        // the entity index + generation version
        // this is used for implementing alive checks
        struct {
            whisker_ecs_entity_index index;
            whisker_ecs_entity_index version;
        };

        // the relationship style A + B
        struct {
            whisker_ecs_entity_index entity_a;
            whisker_ecs_entity_index entity_b;
        };

        // currently reserved and subject to change
        struct {
            uint16_t short1;
            uint16_t short2;
            uint16_t short3;
            uint16_t short4;
        };
    };
} whisker_ecs_entity_id;


// the struct used for an individual entity within the entities array
typedef struct whisker_ecs_entity
{
	// the full id
    whisker_ecs_entity_id id;

    // set when this entity is currently destroyed
    // note: this is not used for alive checks
    bool destroyed;

    // pointer to current name, if any
    char* name;
} whisker_ecs_entity;


// enum specifying the deferred actions possible when doing deferred processing
typedef enum WHISKER_ECS_ENTITY_DEFERRED_ACTION  
{
	WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE,
	WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY,
} WHISKER_ECS_ENTITY_DEFERRED_ACTION;


// struct to hold the deferred action
typedef struct whisker_ecs_entity_deferred_action
{
	whisker_ecs_entity_id id;
	WHISKER_ECS_ENTITY_DEFERRED_ACTION action;
	
} whisker_ecs_entity_deferred_action;


// the main entities struct
typedef struct whisker_ecs_entities
{
	// current list of entities used by the system
	whisker_ecs_entity *entities;
	size_t entities_length;

	// stack of destroyed entities, used when recycling
	whisker_ecs_entity_index *destroyed_entities;
	size_t destroyed_entities_length;

	// dictionary of entity names mapping to indexes
	whisker_ecs_entity_id *entity_names;

	// stack of deferred actions to process
	whisker_ecs_entity_deferred_action *deferred_actions;
	size_t deferred_actions_length;
} whisker_ecs_entities;


// entity struct management functions
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_entities(whisker_ecs_entities **entities);
void whisker_ecs_e_free_entities(whisker_ecs_entities *entities);

// entity management functions
whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities);
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities, whisker_ecs_entity_index *entity_index);
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_new_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_set_name(whisker_ecs_entities *entities, char *name, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_e_create_named(whisker_ecs_entities *entities, char *name);
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_named_(whisker_ecs_entities *entities, char *name, whisker_ecs_entity_id *entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS_ENTITY whisker_ecs_e_process_deferred(whisker_ecs_entities *entities);

// utility functions
whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_e_id(whisker_ecs_entity_id_raw id);
whisker_ecs_entity* whisker_ecs_e_named(whisker_ecs_entities *entities, char* entity_name);
bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
size_t whisker_ecs_e_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_destroyed_count(whisker_ecs_entities *entities);
whisker_ecs_entity_id* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names);
int whisker_ecs_e_compare_entity_ids_(const void *id_a, const void *id_b);
void whisker_ecs_e_sort_entity_array(whisker_ecs_entity_id *entities);

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

