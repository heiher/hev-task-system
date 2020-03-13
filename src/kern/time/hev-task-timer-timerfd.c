/*
 ============================================================================
 Name        : hev-task-timer-timerfd.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Timer Timerfd
 ============================================================================
 */

#if defined(__linux__)

#include "hev-task-timer-timerfd.h"
#include "kern/core/hev-task-system-private.h"
#include "mem/api/hev-memory-allocator-api.h"

HevTaskTimer *
hev_task_timer_new (void)
{
    HevTaskTimerTimerFD *self;
    HevTaskIOReactor *reactor;
    HevTaskIOReactorSetupEvent event;

    self = hev_malloc0 (sizeof (HevTaskTimerTimerFD));
    if (!self)
        return NULL;

    self->fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (self->fd == -1) {
        hev_free (self);
        return NULL;
    }

    reactor = hev_task_system_get_context ()->reactor;
    hev_task_io_reactor_setup_event_set (&event, self->fd,
                                         HEV_TASK_IO_REACTOR_OP_ADD,
                                         HEV_TASK_IO_REACTOR_EV_RO,
                                         &self->base.sched_entity);
    if (hev_task_io_reactor_setup (reactor, &event, 1) == -1) {
        close (self->fd);
        hev_free (self);
        return NULL;
    }

    return &self->base;
}

void
hev_task_timer_destroy (HevTaskTimer *timer)
{
    HevTaskTimerTimerFD *self = (HevTaskTimerTimerFD *)timer;

    close (self->fd);
    hev_free (self);
}

#endif /* defined(__linux__) */
