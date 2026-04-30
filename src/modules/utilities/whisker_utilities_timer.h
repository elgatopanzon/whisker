/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_timer
 * @created     : Tuesday Mar 24, 2026 12:20:09 CST
 * @description : ECS timer utility wrapping standalone timer module
 */

#include "whisker_std.h"
#include "whisker.h"
#include "whisker_timer.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_UTILITIES_TIMER_H
#define WHISKER_UTILITIES_TIMER_H

// timer components
w_ecs_define_component(double, timer_elapsed, 0.0f);
w_ecs_define_component(double, timer_duration, 0.0f);
w_ecs_define_component(int, timer_flags, 0);
w_ecs_define_component(w_entity_id, timer_owner_entity, W_ENTITY_INVALID);
w_ecs_define_component(w_entity_id, timer_finished_comp_id, W_ENTITY_INVALID);
w_ecs_define_component(w_entity_id, timer_entity, W_ENTITY_INVALID);
w_ecs_define_tag(timer_finished);

// create a timer entity with required components
w_entity_id wm_utils_timer_create(struct w_ecs_world *world, w_entity_id owner, const char *name, double duration, bool loop, bool oneshot);

#endif /* WHISKER_UTILITIES_TIMER_H */
