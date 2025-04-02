/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_module_component_change_events
 * @created     : Monday Mar 31, 2025 12:15:41 CST
 */

#include "whisker_std.h"
#include "whisker_macros.h"
#include "whisker_ecs.h"
#include "whisker_ecs_module_event.h"

#ifndef WHISKER_ECS_MODULE_COMPONENT_CHANGE_EVENTS_H
#define WHISKER_ECS_MODULE_COMPONENT_CHANGE_EVENTS_H

/* component change events module
 * this module uses the events module to send component change events.
 * it uses the component deferred actions list and the RESERVED process phase to
 * operate on the list before it's emptied as part of the end of frame operations
 */

void wm_component_change_events_init(struct w_ecs *ecs);

// ECS systems
void wm_component_change_events_system(struct w_sys_context *context);

#endif /* WHISKER_ECS_MODULE_COMPONENT_CHANGE_EVENTS_H */

