/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_random
 * @created     : Wednesday Mar 26, 2026 15:57:00 CST
 * @description : ECS random utility wrapping standalone PCG random module
 */

#include "whisker_std.h"
#include "whisker.h"
#include "whisker_random.h"
#include "whisker_time.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_UTILITIES_RANDOM_H
#define WHISKER_UTILITIES_RANDOM_H

// random components
// PCG state
#define W_RANDOM_COMPONENT_STATE "random_state"
// PCG increment
#define W_RANDOM_COMPONENT_INCREMENT "random_increment"
// min value for int64 output
#define W_RANDOM_COMPONENT_RANGE_MIN "random_range_min"
// max value for int64 output
#define W_RANDOM_COMPONENT_RANGE_MAX "random_range_max"
// last generated int64 value
#define W_RANDOM_COMPONENT_OUTPUT_LONG "random_output_long"
// last generated double value [0.0, 1.0)
#define W_RANDOM_COMPONENT_OUTPUT_DOUBLE "random_output_double"
// points to the owner entity
#define W_RANDOM_COMPONENT_OWNER_ENTITY "random_owner_entity"

// owner random component suffixes
// stores int64 value on the owner
#define W_RANDOM_COMPONENT_VALUE_LONG "_random_value_long"
// stores double value on the owner
#define W_RANDOM_COMPONENT_VALUE_DOUBLE "_random_value_double"
// points to random entity
#define W_RANDOM_COMPONENT_RANDOM_ENTITY "_random_entity"
// pre-registered component ID for long value on owner
#define W_RANDOM_COMPONENT_VALUE_LONG_COMP_ID "random_value_long_comp_id"
// pre-registered component ID for double value on owner
#define W_RANDOM_COMPONENT_VALUE_DOUBLE_COMP_ID "random_value_double_comp_id"

// create full random component name
#define W_RANDOM_QUERY(name, component) name component

w_entity_id wm_utils_random_create(struct w_ecs_world *world, w_entity_id owner, const char *name, int64_t range_min, int64_t range_max);

// get current random int64 value for an owner by name
int64_t wm_utils_random_get_long(struct w_ecs_world *world, w_entity_id owner, const char *name);

// get current random double value for an owner by name
double wm_utils_random_get_double(struct w_ecs_world *world, w_entity_id owner, const char *name);


#endif /* WHISKER_UTILITIES_RANDOM_H */
