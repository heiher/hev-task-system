/*
 ============================================================================
 Name        : hev-task-timer-kevent.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Timer KEvent
 ============================================================================
 */

#ifndef __HEV_TASK_TIMER_KEVENT_H__
#define __HEV_TASK_TIMER_KEVENT_H__

#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "hev-task-timer.h"
#include "kern/core/hev-task-system-private.h"

static inline int
hev_task_timer_set_time (HevTaskTimer *self, const struct timespec *expire)
{
    HevTaskIOReactor *reactor;
    struct kevent event;
    struct timespec curr;
    time_t sec;
    long usec;
    int fd;

    if (clock_gettime (CLOCK_MONOTONIC, &curr) == -1)
        abort ();

    sec = expire->tv_sec - curr.tv_sec;
    usec = (expire->tv_nsec - curr.tv_nsec) / 1000;
    if (usec < 0) {
        sec--;
        usec += 1000000L;
    }
    if (sec < 0)
        usec = 0;
    else
        usec += sec * 1000000L;

    reactor = hev_task_system_get_context ()->reactor;
    fd = hev_task_io_reactor_get_fd (reactor);

    EV_SET (&event, (uintptr_t)self, EVFILT_TIMER, EV_ADD | EV_ONESHOT,
            NOTE_USECONDS, usec, &self->sched_entity);
    return kevent (fd, &event, 1, NULL, 0, NULL);
}

#endif /* __HEV_TASK_TIMER_KEVENT_H__ */
