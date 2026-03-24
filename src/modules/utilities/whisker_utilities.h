/**
 * @author      : ElGatoPanzon
 * @file        : whisker_utilities
 * @created     : Saturday Mar 07, 2026 12:49:55 CST
 * @description : Utility systems and components
 */

#ifndef WHISKER_UTILITIES_H
#define WHISKER_UTILITIES_H

#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "whisker_utilities_timer.h"

// initialize the utilities module
void wm_utils_init(struct w_ecs_world *world);

// cleanup the utilities module
void wm_utils_free(struct w_ecs_world *world);

#endif /* WHISKER_UTILITIES_H */
