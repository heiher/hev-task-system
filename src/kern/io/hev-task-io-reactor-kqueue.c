/*
 ============================================================================
 Name        : hev-task-io-reactor-kqueue.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2022 everyone.
 Description : I/O Reactor KQueue
 ============================================================================
 */

#if !defined(__linux__)

#include "kern/io/hev-task-io-reactor.h"

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

#endif /* !defined(__linux__) */
