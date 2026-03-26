/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_timer
 * @created     : Thursday Mar 26, 2026 13:36:00 CST
 * @description : Standalone timer with loop, oneshot, pause support (no struct version)
 */

#include "whisker_std.h"

#ifndef WHISKER_TIMER_H
#define WHISKER_TIMER_H

// timer state flags
typedef enum w_timer_flags
{
    W_TIMER_FLAG_NONE     = 0,
    W_TIMER_FLAG_LOOP     = 1 << 0,
    W_TIMER_FLAG_ONESHOT  = 1 << 1,
    W_TIMER_FLAG_PAUSED   = 1 << 2,
    W_TIMER_FLAG_FINISHED = 1 << 3,
} w_timer_flags;

// initialize a timer with duration and flags
static inline void w_timer_init(double *elapsed, double *duration, w_timer_flags *flags, double dur, w_timer_flags fl)
{
    *elapsed = 0.0;
    *duration = dur;
    *flags = fl;
}

// update the timer by delta_time, returns true if timer JUST finished this frame
// only returns true on the transition to finished state, not while staying finished
static inline bool w_timer_update(double *elapsed, double duration, w_timer_flags *flags, double delta_time)
{
    if (*flags & W_TIMER_FLAG_PAUSED) return false;
    if (duration <= 0.0) return false;
    if (*flags & W_TIMER_FLAG_FINISHED) return false;  // already finished

    *elapsed += delta_time;

    if (*elapsed >= duration)
    {
        *flags |= W_TIMER_FLAG_FINISHED;
        return true;
    }
    return false;
}

// wrap elapsed for loop timers (call after handling completion)
static inline void w_timer_wrap(double *elapsed, double duration)
{
    if (duration <= 0.0) return;
    while (*elapsed >= duration)
        *elapsed -= duration;
}

// check if timer has finished
static inline bool w_timer_is_finished(w_timer_flags flags)
{
    return (flags & W_TIMER_FLAG_FINISHED) != 0;
}

// clear the finished flag
static inline void w_timer_clear_finished(w_timer_flags *flags)
{
    *flags &= ~W_TIMER_FLAG_FINISHED;
}

// reset the timer to initial state (keeps duration and config flags)
static inline void w_timer_reset(double *elapsed, w_timer_flags *flags)
{
    *elapsed = 0.0;
    *flags &= ~W_TIMER_FLAG_FINISHED;
}

// pause the timer
static inline void w_timer_pause(w_timer_flags *flags)
{
    *flags |= W_TIMER_FLAG_PAUSED;
}

// unpause the timer
static inline void w_timer_unpause(w_timer_flags *flags)
{
    *flags &= ~W_TIMER_FLAG_PAUSED;
}

// check if timer is paused
static inline bool w_timer_is_paused(w_timer_flags flags)
{
    return (flags & W_TIMER_FLAG_PAUSED) != 0;
}

// check if timer is a loop timer
static inline bool w_timer_is_loop(w_timer_flags flags)
{
    return (flags & W_TIMER_FLAG_LOOP) != 0;
}

// check if timer is oneshot
static inline bool w_timer_is_oneshot(w_timer_flags flags)
{
    return (flags & W_TIMER_FLAG_ONESHOT) != 0;
}

// get remaining time until timer finishes
static inline double w_timer_remaining(double elapsed, double duration)
{
    double remaining = duration - elapsed;
    return remaining > 0.0 ? remaining : 0.0;
}

// get progress as ratio 0.0 to 1.0
static inline double w_timer_progress(double elapsed, double duration)
{
    if (duration <= 0.0) return 0.0;
    double progress = elapsed / duration;
    return progress > 1.0 ? 1.0 : progress;
}

#endif /* WHISKER_TIMER_H */
