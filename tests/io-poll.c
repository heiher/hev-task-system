/*
 ============================================================================
 Name        : io-poll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : IO Poll Test
 ============================================================================
 */

#include <stddef.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include <hev-task.h>
#include <hev-task-system.h>
#include <hev-task-io-pipe.h>
#include <hev-task-io-poll.h>

static void
task_entry (void *data)
{
    int fds[2];
    int val;
    HevTaskIOPollFD pfds[2];

    assert (hev_task_io_pipe_pipe (fds) == 0);
    assert (fds[0] >= 0);
    assert (fds[1] >= 0);

    assert (write (fds[1], &val, sizeof (val)) == sizeof (val));

    pfds[0].fd = fds[0];
    pfds[0].events = POLLIN;
    pfds[1].fd = fds[1];
    pfds[1].events = POLLOUT;

    val = hev_task_io_poll (pfds, 2, -1);
    assert (val == 2);
    assert (pfds[0].revents & POLLIN);
    assert (pfds[1].revents & POLLOUT);

    close (fds[0]);
    close (fds[1]);
}

int
main (int argc, char *argv[])
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry, NULL);

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
