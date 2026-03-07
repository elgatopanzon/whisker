/**
 * @author      : ElGatoPanzon
 * @file        : system_disable_startup_phase
 * @created     : Saturday Mar 07, 2026 12:25:40 CST
 * @description : System that disables WM_PHASE_ON_STARTUP after first frame
 */

#ifndef WHISKER_SCHEDULER_DEFAULTS_SYSTEM_DISABLE_STARTUP_PHASE_H
#define WHISKER_SCHEDULER_DEFAULTS_SYSTEM_DISABLE_STARTUP_PHASE_H

#include "whisker.h"

// disables WM_PHASE_ON_STARTUP after first frame
void wm_scheduler_defaults_system_disable_startup_phase_(void *ctx, double delta_time);

#endif /* WHISKER_SCHEDULER_DEFAULTS_SYSTEM_DISABLE_STARTUP_PHASE_H */
