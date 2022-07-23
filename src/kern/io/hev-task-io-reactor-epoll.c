/*
 ============================================================================
 Name        : hev-task-io-reactor-epoll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2022 everyone.
 Description : I/O Reactor EPoll
 ============================================================================
 */

#if defined(__linux__)

#include "kern/io/hev-task-io-reactor.h"

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

#endif /* defined(__linux__) */
