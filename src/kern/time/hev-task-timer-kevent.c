/*
 ============================================================================
 Name        : hev-task-timer-kevent.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Timer KEvent
 ============================================================================
 */

#if !defined(__linux__)

#include "hev-task-timer-kevent.h"
#include "mem/api/hev-memory-allocator-api.h"

HevTaskTimer *
hev_task_timer_new (void)
{
    HevTaskTimer *self;

    self = hev_malloc0 (sizeof (HevTaskTimer));
    if (!self)
        return NULL;

    return self;
}

void
hev_task_timer_destroy (HevTaskTimer *self)
{
    hev_free (self);
}

#endif /* !defined(__linux__) */
