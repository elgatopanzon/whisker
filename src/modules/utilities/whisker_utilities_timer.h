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
// current elapsed time
#define W_TIMER_COMPONENT_ELAPSED "timer_elapsed"
// target duration for this timer until its completed
#define W_TIMER_COMPONENT_DURATION "timer_duration"
// timer state flags (loop, oneshot, paused, finished)
#define W_TIMER_COMPONENT_FLAGS "timer_flags"
// points to the owner entity
#define W_TIMER_COMPONENT_OWNER_ENTITY "timer_owner_entity"
// component ID of the constructed string "NAME_finished" for use on the owner
#define W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID "timer_finished_comp_id"

// owner timer component suffixes
// set on the owner as a tag component when this timer is finished
#define W_TIMER_COMPONENT_FINISHED "_finished_tag"
// points to timer entity
#define W_TIMER_COMPONENT_TIMER_ENTITY "_timer_entity"

// create full timer component name
#define W_TIMER_QUERY(name, component) name component

// create a timer entity with required components
w_entity_id wm_utils_timer_create(struct w_ecs_world *world, w_entity_id owner, const char *name, double duration, bool loop, bool oneshot);

#endif /* WHISKER_UTILITIES_TIMER_H */
