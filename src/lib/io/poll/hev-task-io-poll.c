/*
 ============================================================================
 Name        : hev-task-io-poll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2018 everyone.
 Description :
 ============================================================================
 */

#include "hev-task-io-poll.h"
#include "kern/task/hev-task.h"

int
hev_task_io_poll (HevTaskIOPollFD fds[], unsigned int nfds, int timeout)
{
    HevTask *task;
    unsigned int i;
    int ret;

    ret = poll (fds, nfds, 0);
    if ((ret > 0) || (timeout == 0))
        return ret;

    task = hev_task_self ();
    for (i = 0; i < nfds; i++) {
        if (hev_task_mod_fd (task, fds[i].fd, fds[i].events) == -1)
            hev_task_add_fd (task, fds[i].fd, fds[i].events);
    }

    if (timeout > 0) {
    retry_sleep:
        timeout = hev_task_sleep (timeout);
        ret = poll (fds, nfds, 0);
        if (timeout > 0 && ret == 0)
            goto retry_sleep;

        goto quit;
    }

retry:
    ret = poll (fds, nfds, 0);
    if (ret == 0) {
        hev_task_yield (HEV_TASK_WAITIO);
        goto retry;
    }

quit:
    return ret;
}
