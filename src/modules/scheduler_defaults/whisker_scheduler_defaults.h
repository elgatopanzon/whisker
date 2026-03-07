/**
 * @author      : ElGatoPanzon
 * @file        : whisker_scheduler_defaults
 * @created     : Saturday Mar 07, 2026 11:27:59 CST
 * @description : Default timesteps and phases for the ECS scheduler
 */

#ifndef WHISKER_SCHEDULER_DEFAULTS_H
#define WHISKER_SCHEDULER_DEFAULTS_H

#include "whisker.h"

/* scheduler defaults module
 * this module provides a sane starting point for world scheduling and timestep
 * management:
 * - pre: fine control over pre world state
 * - default: default uncapped timestep for variable frame systems
 * - default fixed: fixed update (defaults to 60.0 hz) for physics
 * - default post: optional secondary post variable timestep for adjustments
 * - default render: the render timestep, runs after all the above
 * - post: fine control over post world state
 * additionally many default phases are added and pre-configured to use these
 * timesteps, making registering systems easier
 */

#ifndef WM_SCHEDULER_DEFAULTS_FIXED_UPDATE_RATE
#define WM_SCHEDULER_DEFAULTS_FIXED_UPDATE_RATE 60.0
#endif /* ifndef WM_SCHEDULER_DEFAULTS_FIXED_UPDATE_RATE */

#ifndef WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED
#define WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED true
#endif /* ifndef WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED */

#ifndef WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED
#define WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED true
#endif /* ifndef WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED */

#ifndef WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED
#define WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED true
#endif /* ifndef WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED */

#ifndef WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED
#define WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED true
#endif /* ifndef WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED */

#ifndef WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
#define WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED true
#endif /* ifndef WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED */

// default timestep IDs (registered in this order)
enum WM_TIMESTEP
{
	WM_TIMESTEP_PRE = 0,
	WM_TIMESTEP_DEFAULT = 1,
	WM_TIMESTEP_DEFAULT_FIXED = 2,
	WM_TIMESTEP_DEFAULT_POST = 3,
	WM_TIMESTEP_DEFAULT_RENDER = 4,
	WM_TIMESTEP_RESERVED = 5,
	WM_TIMESTEP_POST = 6,
};

// default phase IDs (registered in this order)
enum WM_PHASE
{
	WM_PHASE_PRE = 0,
	WM_PHASE_ON_STARTUP = 1,
	WM_PHASE_PRE_LOAD = 2,
	WM_PHASE_ON_LOAD = 3,
	WM_PHASE_POST_LOAD = 4,
	WM_PHASE_PRE_UPDATE = 5,
	WM_PHASE_ON_UPDATE = 6,
	WM_PHASE_POST_UPDATE = 7,
	WM_PHASE_PRE_FIXED_UPDATE = 8,
	WM_PHASE_ON_FIXED_UPDATE = 9,
	WM_PHASE_POST_FIXED_UPDATE = 10,
	WM_PHASE_FINAL_FIXED = 11,
	WM_PHASE_POST_FIXED = 12,
	WM_PHASE_FINAL = 13,
	WM_PHASE_PRE_RENDER = 14,
	WM_PHASE_ON_RENDER = 15,
	WM_PHASE_POST_RENDER = 16,
	WM_PHASE_FINAL_RENDER = 17,
	WM_PHASE_RESERVED = 18,
	WM_PHASE_POST = 19,
};

// register default timesteps and phases on the world scheduler
// fixed_update_rate: Hz for the fixed timestep (defaults to 60.0 if <= 0)
void wm_scheduler_defaults_init(struct w_ecs_world *world, double fixed_update_rate);

// cleanup (no-op, scheduler owns the data)
void wm_scheduler_defaults_free(struct w_ecs_world *world);

#endif /* WHISKER_SCHEDULER_DEFAULTS_H */
