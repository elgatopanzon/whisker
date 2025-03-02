/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_types_base
 * @created     : Sunday Mar 02, 2025 15:39:35 CST
 */

#include "whisker_std.h"

#ifndef WHISKER_ECS_TYPES_BASE_H
#define WHISKER_ECS_TYPES_BASE_H

////////////////////
//  entity types  //
////////////////////

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

#endif /* WHISKER_ECS_TYPES_BASE_H */

