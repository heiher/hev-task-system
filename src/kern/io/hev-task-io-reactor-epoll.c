/*
 ============================================================================
 Name        : hev-task-io-reactor-epoll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : I/O Reactor EPoll
 ============================================================================
 */

#if defined(__linux__)

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

    self->fd = epoll_create (128);
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
    int i, res = 0;

    for (i = 0; i < count; i++) {
        HevTaskIOReactorSetupEvent *ev = &events[i];
        res |= epoll_ctl (self->fd, ev->op, ev->fd, &ev->event);
    }

    return res;
}

int
hev_task_io_reactor_wait (HevTaskIOReactor *self,
                          HevTaskIOReactorWaitEvent *events, int count,
                          int timeout)
{
    return epoll_wait (self->fd, events, count, timeout);
}

#endif /* defined(__linux__) */
