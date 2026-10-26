/*
 ============================================================================
 Name        : task-exit.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2026 everyone.
 Description : Task Exit Test
 ============================================================================
 */

#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>

#include <hev-sys.h>
#include <hev-task.h>
#include <hev-task-system.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#define LOOP_COUNT (100)
#define MAX_FD (1024)

static int fds[2];

static int
count_fds (void)
{
    int fd, count = 0;

    for (fd = 0; fd < MAX_FD; fd++)
        if (fcntl (fd, F_GETFD) >= 0)
            count++;

    return count;
}

static void
task_entry (void *data)
{
    hev_task_exit ();
    assert (0);
}

static void
task_sleep_entry (void *data)
{
    /* exit after a pending timer */
    hev_task_sleep (1);
    hev_task_exit ();
    assert (0);
}

static void
task_reader_entry (void *data)
{
    HevTask *task = hev_task_self ();
    int val;

    assert (hev_task_io_socket_socketpair (PF_LOCAL, SOCK_STREAM, 0, fds) == 0);
    assert (hev_task_add_fd (task, fds[0], POLLIN) == 0);

    /* blocked on I/O */
    assert (hev_task_io_read (fds[0], &val, sizeof (val), NULL, NULL) > 0);

    /* exit with open fds registered, closed fds are removed from reactor */
    close (fds[0]);
    close (fds[1]);
    hev_task_exit ();
    assert (0);
}

static void
task_writer_entry (void *data)
{
    HevTask *task = hev_task_self ();
    int val = 0;

    assert (hev_task_add_fd (task, fds[1], POLLOUT) == 0);
    assert (hev_task_io_write (fds[1], &val, sizeof (val), NULL, NULL) > 0);
    assert (hev_task_del_fd (task, fds[1]) == 0);
}

static void
run (void)
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_sleep_entry, NULL);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_reader_entry, NULL);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry, NULL);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_writer_entry, NULL);

    hev_task_system_run ();

    hev_task_system_fini ();
}

int
main (int argc, char *argv[])
{
    int i, count;

    count = count_fds ();

    for (i = 0; i < LOOP_COUNT; i++)
        run ();

    assert (count_fds () == count);

    return 0;
}
