/**
 * @author      : ElGatoPanzon
 * @file        : benchmark_whisker_time
 * @created     : 2026-02-26
 * @description : benchmarks comparing w_time_precise vs clock_gettime overhead
 */

#include "ubench.h"
#include "whisker_time.h"
#include <time.h>

UBENCH(bench_time_precise, clock_gettime_raw)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	(void)ts;  // prevent optimization
}

UBENCH(bench_time_precise, w_time_precise)
{
	uint64_t t = w_time_precise();
	(void)t;
}

UBENCH_MAIN();
