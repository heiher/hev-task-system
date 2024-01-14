/*
 ============================================================================
 Name        : cio-fd.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : CIO Fd Test
 ============================================================================
 */

#include <assert.h>
#include <string.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-pipe.h>
#include <hev-task-cio-fd.h>
#include <hev-task-system.h>

static int fds1[4];
static int fds2[4];

static void
task_splice_entry (void *data)
{
    HevTask *task = hev_task_self ();
    HevTaskCIO *a, *b;

    assert (hev_task_add_fd (task, fds1[1], POLLOUT) == 0);
    assert (hev_task_add_fd (task, fds2[1], POLLOUT) == 0);
    assert (hev_task_add_fd (task, fds1[2], POLLIN) == 0);
    assert (hev_task_add_fd (task, fds2[2], POLLIN) == 0);

    a = HEV_TASK_CIO (hev_task_cio_fd_new (fds1[2], fds1[1]));
    assert (a != NULL);
    b = HEV_TASK_CIO (hev_task_cio_fd_new (fds2[2], fds2[1]));
    assert (b != NULL);

    assert (hev_task_cio_get_fd (a, 0) == fds1[2]);
    assert (hev_task_cio_get_fd (a, 1) == fds1[1]);
    assert (hev_task_cio_get_fd (b, 0) == fds2[2]);
    assert (hev_task_cio_get_fd (b, 1) == fds2[1]);

    hev_task_cio_splice (a, b, 2048, NULL, NULL);

    hev_object_unref (HEV_OBJECT (a));
    hev_object_unref (HEV_OBJECT (b));
}

static void
task_entry (void *data)
{
    HevTask *task = hev_task_self ();
    HevTaskCIO *a, *b;
    struct iovec iov;
    char buf1[64];
    char buf2[64];
    ssize_t size;
    int res;

    res = hev_task_io_pipe_pipe (&fds1[0]);
    assert (res == 0);
    res = hev_task_io_pipe_pipe (&fds1[2]);
    assert (res == 0);
    res = hev_task_io_pipe_pipe (&fds2[0]);
    assert (res == 0);
    res = hev_task_io_pipe_pipe (&fds2[2]);
    assert (res == 0);

    assert (hev_task_add_fd (task, fds1[3], POLLOUT) == 0);
    assert (hev_task_add_fd (task, fds2[3], POLLOUT) == 0);
    assert (hev_task_add_fd (task, fds1[0], POLLIN) == 0);
    assert (hev_task_add_fd (task, fds2[0], POLLIN) == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_splice_entry, NULL);

    a = HEV_TASK_CIO (hev_task_cio_fd_new (fds1[0], fds1[3]));
    assert (a != NULL);
    b = HEV_TASK_CIO (hev_task_cio_fd_new (fds2[0], fds2[3]));
    assert (b != NULL);

    size = hev_task_cio_write_exact (a, buf1, 64, NULL, NULL);
    assert (size == 64);
    res = hev_task_cio_flush (a, NULL, NULL);
    assert (res == 0);
    size = hev_task_cio_read_exact (b, buf2, 64, NULL, NULL);
    assert (size == 64);
    assert (memcmp (buf1, buf2, 64) == 0);

    iov.iov_base = buf1;
    iov.iov_len = 64;
    size = hev_task_cio_writev_exact (a, &iov, 1, NULL, NULL);
    assert (size == 64);
    res = hev_task_cio_flush (a, NULL, NULL);
    assert (res == 0);
    iov.iov_base = buf2;
    iov.iov_len = 64;
    size = hev_task_cio_readv_exact (b, &iov, 1, NULL, NULL);
    assert (size == 64);
    assert (memcmp (buf1, buf2, 64) == 0);

    assert (hev_task_cio_read_dgram (a, buf1, 64, NULL, NULL, NULL) == -1);
    assert (hev_task_cio_readv_dgram (a, &iov, 1, NULL, NULL, NULL) == -1);
    assert (hev_task_cio_write_dgram (a, buf1, 64, NULL, NULL, NULL) == -1);
    assert (hev_task_cio_writev_dgram (a, &iov, 1, NULL, NULL, NULL) == -1);

    hev_object_unref (HEV_OBJECT (a));
    hev_object_unref (HEV_OBJECT (b));
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
