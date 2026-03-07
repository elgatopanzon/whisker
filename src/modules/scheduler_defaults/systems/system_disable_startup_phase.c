/**
 * @author      : ElGatoPanzon
 * @file        : system_disable_startup_phase
 * @created     : Saturday Mar 07, 2026 12:25:40 CST
 * @description : System that disables WM_PHASE_ON_STARTUP after first frame
 */

#include "system_disable_startup_phase.h"
#include "../whisker_scheduler_defaults.h"

// disables WM_PHASE_ON_STARTUP after first frame
void wm_scheduler_defaults_system_disable_startup_phase_(void *ctx, double delta_time)
{
	(void)delta_time;
	struct w_ecs_world *world = ctx;
	w_ecs_set_system_phase_state(world, WM_PHASE_ON_STARTUP, false);
}
