/*
 ============================================================================
 Name        : hev-task-io-poll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2019 everyone.
 Description :
 ============================================================================
 */

#if defined(__linux__) && defined(ENABLE_IO_RING)
#include <errno.h>
#include <linux/io_uring.h>
#endif

#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/io/ring/hev-task-io-ring.h"
#include "lib/misc/hev-compiler.h"

#include "hev-task-io-poll.h"

EXPORT_SYMBOL int
hev_task_io_poll (HevTaskIOPollFD fds[], unsigned int nfds, int timeout)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;

    if (hev_task_io_ring_is_enabled () && (timeout != 0)) {
        struct io_uring_cqe cqes[nfds];
        unsigned int i, j, count = 0;
        int remain = timeout;

        for (i = 0; i < nfds; i++) {
            sqe = hev_task_io_ring_get_sqe ();
            sqe->opcode = IORING_OP_POLL_ADD;
            sqe->fd = fds[i].fd;
            sqe->poll_events = fds[i].events;
            hev_task_io_ring_submit (sqe, &cqes[i]);
        }

        if (timeout > 0) {
            do {
                remain = hev_task_sleep (remain);

                count = 0;
                for (j = 0; j < nfds; j++) {
                    if (!cqes[j].user_data)
                        count++;
                }
            } while (remain > 0 && count < nfds);
        } else {
            do {
                hev_task_yield (HEV_TASK_WAITIO);

                count = 0;
                for (j = 0; j < nfds; j++) {
                    if (!cqes[j].user_data)
                        count++;
                }
            } while (count < nfds);
        }

        count = 0;
        for (i = 0; i < nfds; i++) {
            if (cqes[i].user_data) {
                hev_task_io_ring_cancel (&cqes[i]);
                fds[i].revents = 0;
                continue;
            }

            fds[i].revents = cqes[i].res < 0 ? POLLNVAL : cqes[i].res;
            if (fds[i].revents)
                count++;
        }

        return count;
    }
#endif

    int res;

    if (timeout == 0) {
        res = poll (fds, nfds, 0);
    } else {
        HevTask *task = hev_task_self ();
        unsigned int i;

        for (i = 0; i < nfds; i++) {
            if (hev_task_mod_fd (task, fds[i].fd, fds[i].events) < 0)
                hev_task_add_fd (task, fds[i].fd, fds[i].events);
        }

        if (timeout > 0) {
            do {
                timeout = hev_task_sleep (timeout);
                res = poll (fds, nfds, 0);
            } while (timeout > 0 && res == 0);
        } else {
            do {
                hev_task_yield (HEV_TASK_WAITIO);
                res = poll (fds, nfds, 0);
            } while (res == 0);
        }
    }

    return res;
}
