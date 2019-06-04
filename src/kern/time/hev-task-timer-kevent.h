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
    long nsec;
    int fd;

    if (clock_gettime (CLOCK_MONOTONIC, &curr) == -1)
        abort ();

    sec = expire->tv_sec - curr.tv_sec;
    nsec = expire->tv_nsec - curr.tv_nsec;
    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }
    nsec += sec * 1000000000L;

    reactor = hev_task_system_get_context ()->reactor;
    fd = hev_task_io_reactor_get_fd (reactor);

    EV_SET (&event, (uintptr_t)self, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
    kevent (fd, &event, 1, NULL, 0, NULL);

    EV_SET (&event, (uintptr_t)self, EVFILT_TIMER, EV_ADD | EV_ONESHOT,
            NOTE_NSECONDS, nsec, &self->sched_entity);
    return kevent (fd, &event, 1, NULL, 0, NULL);
}

#endif /* __HEV_TASK_TIMER_KEVENT_H__ */
