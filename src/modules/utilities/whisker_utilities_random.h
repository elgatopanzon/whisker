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
w_ecs_define_component(uint64_t, random_state, 0);
// PCG increment
w_ecs_define_component(uint64_t, random_increment, 0);
// min value for int64 output
w_ecs_define_component(int64_t, random_range_min, 0);
// max value for int64 output
w_ecs_define_component(int64_t, random_range_max, 0);
// last generated int64 value
w_ecs_define_component(int64_t, random_output_long, 0);
// last generated double value [0.0, 1.0)
w_ecs_define_component(double, random_output_double, 0.0f);
// points to the owner entity
w_ecs_define_component(w_entity_id, random_owner_entity, W_ENTITY_INVALID);
// pre-registered component ID for long value on owner
w_ecs_define_component(w_entity_id, random_output_long_comp_id, W_ENTITY_INVALID);
// pre-registered component ID for double value on owner
w_ecs_define_component(w_entity_id, random_output_double_comp_id, W_ENTITY_INVALID);
w_ecs_define_component(w_entity_id, random_entity, W_ENTITY_INVALID);
#define W_RANDOM_COMPONENT_RANDOM_ENTITY "_random_entity"

// create a random generator entity
w_entity_id wm_utils_random_create(struct w_ecs_world *world, w_entity_id owner, const char *name, int64_t range_min, int64_t range_max);

// get current random int64 value for an owner by name
inline int64_t wm_utils_random_get_long(struct w_ecs_world *world, w_entity_id owner, const char *name);

// get current random double value for an owner by name
inline double wm_utils_random_get_double(struct w_ecs_world *world, w_entity_id owner, const char *name);


#endif /* WHISKER_UTILITIES_RANDOM_H */
