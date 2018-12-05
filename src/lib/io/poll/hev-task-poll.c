/*
 ============================================================================
 Name        : hev-task-poll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include "hev-task-poll.h"
#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io-shared.h"

int
hev_task_poll (HevTaskPollFD fds[], unsigned int nfds, int timeout)
{
    unsigned int i;
    int ret;

    ret = poll (fds, nfds, 0);
    if ((ret > 0) || (timeout == 0))
        return ret;

    for (i = 0; i < nfds; i++) {
        HevTaskIOReactorEvents event = 0;
        if (fds[i].events & POLLIN)
            event |= HEV_TASK_IO_REACTOR_EV_RO;
        if (fds[i].events & POLLOUT)
            event |= HEV_TASK_IO_REACTOR_EV_WO;
        hev_task_io_res_fd (fds[i].fd, event);
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
