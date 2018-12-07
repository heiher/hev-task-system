/*
 ============================================================================
 Name        : hev-task-io-reactor-kqueue.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : I/O Reactor KQueue
 ============================================================================
 */

#if !defined(__linux__)

#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "kern/io/hev-task-io-reactor.h"
#include "mm/api/hev-memory-allocator-api.h"

HevTaskIOReactor *
hev_task_io_reactor_new (void)
{
    HevTaskIOReactor *self;
    int flags;

    self = hev_malloc0 (sizeof (HevTaskIOReactorKQueue));
    if (!self)
        return NULL;

    self->fd = kqueue ();
    if (self->fd == -1) {
        hev_free (self);
        return NULL;
    }

    flags = fcntl (self->fd, F_GETFD);
    if (flags == -1) {
        hev_free (self);
        return NULL;
    }

    flags |= FD_CLOEXEC;
    if (fcntl (self->fd, F_SETFD, flags) == -1) {
        hev_free (self);
        return NULL;
    }

    return self;
}

void
hev_task_io_reactor_destroy (HevTaskIOReactor *reactor)
{
    HevTaskIOReactorKQueue *self = (HevTaskIOReactorKQueue *)reactor;

    if (self->count > SEVENT_COUNT)
        hev_free (self->events);
    close (reactor->fd);
    hev_free (self);
}

int
hev_task_io_reactor_setup (HevTaskIOReactor *reactor,
                           HevTaskIOReactorSetupEvent *events, int count)
{
    HevTaskIOReactorKQueue *self = (HevTaskIOReactorKQueue *)reactor;
    int res = 0;
    size_t size;

    if (self->count) {
        res = kevent (reactor->fd, self->events, self->count, NULL, 0, NULL);
        if (self->count > SEVENT_COUNT)
            hev_free (self->events);
    }

    size = sizeof (HevTaskIOReactorSetupEvent) * count;
    if (count > SEVENT_COUNT) {
        self->events = hev_malloc (size);
        if (!self->events)
            return -1;
    } else {
        self->events = self->sevents;
    }

    memcpy (self->events, events, size);
    self->count = count;

    return res;
}

int
hev_task_io_reactor_wait (HevTaskIOReactor *reactor,
                          HevTaskIOReactorWaitEvent *events, int count,
                          int timeout)
{
    HevTaskIOReactorKQueue *self = (HevTaskIOReactorKQueue *)reactor;
    struct timespec tsz = { 0 };
    struct timespec *tsp = NULL;
    int res;

    if (timeout >= 0)
        tsp = &tsz;

    res = kevent (reactor->fd, self->events, self->count, events, count, tsp);
    if (self->count) {
        if (self->count > SEVENT_COUNT)
            hev_free (self->events);
        self->count = 0;
    }

    return res;
}

#endif /* !defined(__linux__) */
