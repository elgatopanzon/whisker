/**
 * @author      : ElGatoPanzon
 * @file        : system_enable_restart_shutdown_phases
 * @created     : Sunday Apr 27, 2026 16:30:00 CST
 * @description : System that enables restart/shutdown phases mid-update and triggers scheduler rebuild
 */

#ifndef WHISKER_SCHEDULER_DEFAULTS_SYSTEM_ENABLE_RESTART_SHUTDOWN_PHASES_H
#define WHISKER_SCHEDULER_DEFAULTS_SYSTEM_ENABLE_RESTART_SHUTDOWN_PHASES_H

#include "whisker.h"

// checks world->update_result for restart/shutdown state and enables the
// appropriate phase, then forces a scheduler rebuild to append those phases
// to the current update iteration
void wm_scheduler_defaults_system_enable_restart_shutdown_phases_(void *ctx, double delta_time);

#endif /* WHISKER_SCHEDULER_DEFAULTS_SYSTEM_ENABLE_RESTART_SHUTDOWN_PHASES_H */
