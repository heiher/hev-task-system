/*
 ============================================================================
 Name        : hev-task-io-reactor.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2022 everyone.
 Description : I/O Reactor
 ============================================================================
 */

#include <unistd.h>
#include <fcntl.h>

#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-io-reactor.h"

HevTaskIOReactor *
hev_task_io_reactor_new (void)
{
    HevTaskIOReactor *self;
    int flags;

    self = hev_malloc0 (sizeof (HevTaskIOReactor));
    if (!self)
        return NULL;

    self->fd = hev_task_io_reactor_open ();
    if (self->fd < 0) {
        hev_free (self);
        return NULL;
    }

    flags = fcntl (self->fd, F_GETFD);
    if (flags < 0) {
        hev_free (self);
        return NULL;
    }

    flags |= FD_CLOEXEC;
    if (fcntl (self->fd, F_SETFD, flags) < 0) {
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
hev_task_io_reactor_get_fd (HevTaskIOReactor *self)
{
    return self->fd;
}
