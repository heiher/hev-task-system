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
#include "mm/api/hev-memory-allocator-api.h"

HevTaskIOReactor *
hev_task_io_reactor_new (void)
{
    HevTaskIOReactor *self;
    int flags;

    self = hev_malloc0 (sizeof (HevTaskIOReactor));
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
hev_task_io_reactor_destroy (HevTaskIOReactor *self)
{
    close (self->fd);
    hev_free (self);
}

int
hev_task_io_reactor_setup (HevTaskIOReactor *self,
                           HevTaskIOReactorSetupEvent *events, int count)
{
    return kevent (self->fd, events, count, NULL, 0, NULL);
}

int
hev_task_io_reactor_wait (HevTaskIOReactor *self,
                          HevTaskIOReactorWaitEvent *events, int count,
                          int timeout)
{
    if (timeout >= 0) {
        struct timespec ts = { 0 };

        return kevent (self->fd, NULL, 0, events, count, &ts);
    }

    return kevent (self->fd, NULL, 0, events, count, NULL);
}

#endif /* !defined(__linux__) */
