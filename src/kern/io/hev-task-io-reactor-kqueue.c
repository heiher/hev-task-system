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
#include <fcntl.h>

#include "kern/io/hev-task-io-reactor.h"
#include "mem/api/hev-memory-allocator-api.h"

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

    close (reactor->fd);
    hev_free (self);
}

int
hev_task_io_reactor_setup (HevTaskIOReactor *reactor,
                           HevTaskIOReactorSetupEvent *events, int count)
{
    int i, res = -1;

    for (i = 0; i < count; i++) {
        if (!(events[i].flags & EV_DELETE)) {
            res = kevent (reactor->fd, &events[i], count - i, NULL, 0, NULL);
            break;
        }
        res &= kevent (reactor->fd, &events[i], 1, NULL, 0, NULL);
    }

    return res;
}

int
hev_task_io_reactor_wait (HevTaskIOReactor *reactor,
                          HevTaskIOReactorWaitEvent *events, int count,
                          int timeout)
{
    struct timespec tsz = { 0 };
    struct timespec *tsp = NULL;

    if (timeout >= 0)
        tsp = &tsz;

    return kevent (reactor->fd, NULL, 0, events, count, tsp);
}

#endif /* !defined(__linux__) */
