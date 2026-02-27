/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_time
 * @created     : Thursday Mar 06, 2025 11:41:03 CST
 * @description : precise timer and fixed-timestep implementation
 */

#include "whisker_std.h"
#include "whisker_debug.h"

#include "whisker_time.h"

#include <stdio.h>

// windows implementation of clock_gettime()
#if defined(_WIN32) || defined(_WIN64)
// int clock_gettime(int, struct timespec *spec)
// {
// 	__int64 wintime;
// 	GetSystemTimeAsFileTime((FILETIME*)&wintime);
// 	wintime      -=116444736000000000i64;  //1jan1601 to 1jan1970
// 	spec->tv_sec  =wintime / 10000000i64;
// 	spec->tv_nsec =wintime % 10000000i64 *100;
// 	return 0;
// }
#endif

// get precise time in nanoseconds
uint64_t w_time_precise()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (uint64_t)time.tv_sec * WHISKER_TIME_RESOLUTION + (uint64_t)time.tv_nsec;
}

/*************************
*  time step functions  *
*************************/
void w_debug_print_time_step(w_time_step *time_step)
{
	debug_printf("debug: time_step: \n");
	debug_printf("debug: time_step.update_rate_sec: %f\n", time_step->update_rate_sec);
	debug_printf("debug: time_step.update_multiplier: %d\n", time_step->update_multiplier);
	debug_printf("debug: time_step.update_time_target: %zu\n", time_step->update_time_target);
	debug_printf("debug: time_step.update_time_prev: %zu\n", time_step->update_time_prev);
	debug_printf("debug: time_step.update_time_current: %zu\n", time_step->update_time_current);
	debug_printf("debug: time_step.time_resolution: %zu\n", time_step->time_resolution);
	debug_printf("debug: time_step.delta_snap_base: %zu\n", time_step->delta_snap_base);
	debug_printf("debug: time_step.delta_snap_max_error: %zu\n", time_step->delta_snap_max_error);
	debug_printf("debug: time_step.delta_averages: ");
	for (int i = 0; i < WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT; ++i)
	{
		debug_printf("%zu ", time_step->delta_averages[i]);
	}
	debug_printf("\n");
	debug_printf("debug: time_step.delta_average_residual: %zu\n", time_step->delta_average_residual);
	debug_printf("debug: time_step.delta_accumulation: %zu\n", time_step->delta_accumulation);
	debug_printf("debug: time_step.delta_time_fixed: %f\n", time_step->delta_time_fixed);
	debug_printf("debug: time_step.delta_time_real: %zu\n", time_step->delta_time_real);
	debug_printf("debug: time_step.delta_time_variable: %f\n", time_step->delta_time_variable);
	debug_printf("debug: time_step.tick_count: %zu\n", time_step->tick_count);
	debug_printf("");
}

// create and init a whisker_time_step struct to be used with the time step
// should update function
w_time_step w_time_step_create(double update_rate_sec, int update_count_max, bool uncapped, bool delta_clamp_enabled, bool delta_snap_enabled, bool delta_average_enabled, bool delta_accumulation_enabled, bool delta_accumulation_clamp_enabled)
{
	w_time_step time_step = {
		.delta_clamp_enabled = delta_clamp_enabled,
		.delta_snap_enabled = delta_snap_enabled,
		.delta_average_enabled = delta_average_enabled,
		.delta_accumulation_enabled = delta_accumulation_enabled,
		.delta_accumulation_clamp_enabled = delta_accumulation_clamp_enabled,
		.update_count_max = update_count_max,
		.time_resolution = WHISKER_TIME_RESOLUTION,

		// this is technically the display refresh rate, but it acts as a snap base
		// value as well
		.delta_snap_base = 60,
		.uncapped = uncapped,
		.update_multiplier = 1,
	};

	time_step.delta_snap_max_error = time_step.time_resolution * WHISKER_TIME_STEP_SNAP_ERROR_MULTIPLIER;

	w_time_step_set_rate(&time_step, update_rate_sec);

	time_step.update_time_current = 0;
	time_step.delta_accumulation = 0;
	time_step.update_time_prev = w_time_precise();

	return time_step;
}

// step the time step and return the desired number of updates to do
int w_time_step_advance(w_time_step *time_step)
{
	w_time_step_do_step_(time_step);
	// whisker_debug_print_time_step(time_step);

	// always return update in uncapped and override delta_time to variable time
	if (time_step->uncapped)
	{
		time_step->delta_time_fixed = time_step->delta_time_variable;
		return 1;
	}

	// calculate update count using delta accumulation
	int update_count = 0;
	while (time_step->delta_accumulation >= time_step->update_time_target * time_step->update_multiplier)
	{
		time_step->delta_accumulation -= time_step->update_time_target;
		update_count++;
	}

	// if update count max is > 0, clamp update count to max
	update_count = (time_step->update_count_max > 0 && update_count > time_step->update_count_max) ? time_step->update_count_max : update_count;

	time_step->update_tick_count += update_count;

	return update_count;
}

// advance time step on whisker_time_step struct
void w_time_step_do_step_(w_time_step *time_step)
{
	// update delta time
	time_step->update_time_current = w_time_precise();
	time_step->delta_time_real = time_step->update_time_current - time_step->update_time_prev;
	time_step->update_time_prev = time_step->update_time_current;

	// update tick count
	time_step->tick_count += time_step->delta_time_real;

	if (time_step->delta_clamp_enabled && !time_step->uncapped)
	{
		// clamp values in odd circumstances
		if (time_step->delta_time_real > time_step->update_time_target * WHISKER_TIME_DELTA_SNAP_FREQ_COUNT)
		{
			time_step->delta_time_real = WHISKER_TIME_DELTA_SNAP_FREQ_COUNT;
		}
	}
	if (time_step->delta_snap_enabled && !time_step->uncapped)
	{
		// snap delta time to nearest frequency
		for (int i = 0; i < WHISKER_TIME_DELTA_SNAP_FREQ_COUNT; ++i)
		{
			if (llabs((int64_t)(time_step->delta_time_real - time_step->delta_snap_frequencies[i])) < time_step->delta_snap_max_error)
			{
				time_step->delta_time_real = time_step->delta_snap_frequencies[i];
				break;
			}
		}
	}
	if (time_step->delta_average_enabled && !time_step->uncapped)
	{

		// update the delta time averages and set delta time
		for (int i = 0; i < WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT - 1; ++i)
		{
			time_step->delta_averages[i] = time_step->delta_averages[i+1];
		}
		time_step->delta_averages[WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT - 1] = time_step->delta_time_real;

		// calculate the delta time average
		int64_t delta_average_sum = 0;
		for (int i = 0; i < WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT; ++i)
		{
			delta_average_sum += time_step->delta_averages[i];
		}
		time_step->delta_time_real = delta_average_sum / WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT;

		// adjust delta time and update average residual
		time_step->delta_average_residual += delta_average_sum % WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT;
		time_step->delta_time_real += time_step->delta_average_residual / WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT;
		time_step->delta_average_residual %= WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT;
	}
	if (time_step->delta_accumulation_enabled && !time_step->uncapped)
	{
		// add the delta time to the delta accumulation
		time_step->delta_accumulation += time_step->delta_time_real;

		// prevent update accumulation spiral of death
		if (time_step->delta_accumulation_clamp_enabled && time_step->delta_accumulation > time_step->update_time_target * WHISKER_TIME_DELTA_SNAP_FREQ_COUNT)
		{
			time_step->delta_accumulation = 0;
			time_step->delta_time_real = time_step->update_time_target;
		}
	}

	// set variable delta time
	time_step->delta_time_variable = ((double)time_step->delta_time_real) / WHISKER_TIME_RESOLUTION;
}

// set the desired update rate in seconds
void w_time_step_set_rate(w_time_step *time_step, double update_rate_sec)
{
	time_step->update_rate_sec = update_rate_sec;

	// calculate fixed delta time
	time_step->delta_time_fixed = 1.0 / update_rate_sec;

	// calculate desired update time based on time resolution
	time_step->update_time_target = time_step->time_resolution / update_rate_sec;

	// set snap frequencies based on delta snap base
	for (int i = 0; i < WHISKER_TIME_DELTA_SNAP_FREQ_COUNT; ++i)
	{
		time_step->delta_snap_frequencies[i] = (time_step->time_resolution / time_step->delta_snap_base) * (i + 1);
	}

	// fill the delta averages samples with the desired frametime target
	for (int i = 0; i < WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT; ++i)
	{
		time_step->delta_averages[i] = time_step->update_time_target;
	}
	time_step->delta_average_residual = 0;
}
