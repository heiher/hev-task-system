/*
 ============================================================================
 Name        : hev-task-timer-timerfd.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Timer Timerfd
 ============================================================================
 */

#ifndef __HEV_TASK_TIMER_TIMER_FD_H__
#define __HEV_TASK_TIMER_TIMER_FD_H__

#include <stdlib.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include "hev-task-timer.h"

typedef struct _HevTaskTimerTimerFD HevTaskTimerTimerFD;

struct _HevTaskTimerTimerFD
{
    HevTaskTimer base;

    int fd;
};

static inline int
hev_task_timer_fast_check (HevTaskTimer *timer)
{
    HevTaskTimerTimerFD *self = (HevTaskTimerTimerFD *)timer;
    uint64_t time;

    return read (self->fd, &time, sizeof (time)) == sizeof (time);
}

static inline int
hev_task_timer_set_time (HevTaskTimer *timer, const struct timespec *expire)
{
    HevTaskTimerTimerFD *self = (HevTaskTimerTimerFD *)timer;
    struct itimerspec sp;

    sp.it_value = *expire;
    sp.it_interval.tv_sec = 0;
    sp.it_interval.tv_nsec = 0;

    return timerfd_settime (self->fd, TFD_TIMER_ABSTIME, &sp, NULL);
}

static inline unsigned int
hev_task_timer_get_time (HevTaskTimer *timer, const struct timespec *expire)
{
    HevTaskTimerTimerFD *self = (HevTaskTimerTimerFD *)timer;
    struct itimerspec sp;

    if (timerfd_gettime (self->fd, &sp) == -1)
        abort ();

    if ((sp.it_value.tv_sec + sp.it_value.tv_nsec) == 0)
        return 0;

    return (sp.it_value.tv_sec * 1000000) + (sp.it_value.tv_nsec / 1000);
}

#endif /* __HEV_TASK_TIMER_TIMER_FD_H__ */
