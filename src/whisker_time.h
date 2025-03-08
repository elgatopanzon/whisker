/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_time
 * @created     : Thursday Mar 06, 2025 11:40:56 CST
 */

#include "whisker_std.h"

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#ifndef WHISKER_TIME_H
#define WHISKER_TIME_H

#define WHISKER_TIME_RESOLUTION 1000000000ULL
#define WHISKER_TIME_STEP_SNAP_ERROR_MULTIPLIER 0.0002
#define WHISKER_TIME_DELTA_SNAP_FREQ_COUNT 8
#define WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT 4

// platform specific time libraries
#if defined(_WIN32) || defined(_WIN64)
// #include <windows.h>
// struct timespec { long tv_sec; long tv_nsec; };
// int clock_gettime(int, struct timespec *spec);
#else
#include <sys/time.h>
#endif


// struct holding time step config
typedef struct whisker_time_step
{
	// the desired ticks per second
	double update_rate_sec;
	bool uncapped;

	// used to enable multiplicity
	int update_multiplier;

	// the time resolution value to use
	uint64_t time_resolution;

	// calculated delta time
	double delta_time_fixed;

	// calculated update target
	uint64_t update_time_target;

	// max error detection for delta snapping
	uint64_t delta_snap_max_error;

	// list of frequencies to snap delta time to
	uint64_t delta_snap_base;
	uint64_t delta_snap_frequencies[WHISKER_TIME_DELTA_SNAP_FREQ_COUNT];

	// delta average list
	uint64_t delta_averages[WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT];
	uint64_t delta_average_residual;

	// running state values
	uint64_t update_time_current;
	uint64_t update_time_prev;
	uint64_t delta_accumulation;
	uint64_t delta_time_real;
	double delta_time_variable;

	// running stats
	uint64_t tick_count;
} whisker_time_step;

// time functions
uint64_t whisker_time_get_precise_time();

// time step functions
whisker_time_step whisker_time_step_create(double update_rate_sec, bool uncapped);
int whisker_time_step_step_get_update_count(whisker_time_step *time_step);
void whisker_time_step_do_step(whisker_time_step *time_step);
void whisker_time_step_set_update_rate(whisker_time_step *time_step, double update_rate_sec);

#endif /* WHISKER_TIME_H */

