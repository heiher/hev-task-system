/*
 ============================================================================
 Name        : cio-null.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : CIO Null Tests
 ============================================================================
 */

#include <assert.h>
#include <stddef.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-system.h>
#include <hev-task-cio-null.h>

static void
task_entry (void *data)
{
    HevTaskCIO *a, *b, *c, *d;
    struct iovec iov;
    char buf[16];

    iov.iov_base = buf;
    iov.iov_len = 16;

    a = HEV_TASK_CIO (hev_task_cio_null_new ());
    assert (a != NULL);

    b = HEV_TASK_CIO (hev_task_cio_null_new ());
    assert (b != NULL);

    c = HEV_TASK_CIO (hev_task_cio_null_new ());
    assert (c != NULL);

    d = HEV_TASK_CIO (hev_task_cio_null_new ());
    assert (d != NULL);

    assert (hev_task_cio_push (b, a) == b);
    assert (hev_task_cio_push (c, b) == c);

    assert (hev_task_cio_get_fd (c, 0) == -1);
    assert (hev_task_cio_get_fd (c, 1) == -1);

    assert (hev_task_cio_read (c, buf, 16, NULL, NULL) == 0);
    assert (hev_task_cio_readv (c, &iov, 1, NULL, NULL) == 0);
    assert (hev_task_cio_write (c, buf, 16, NULL, NULL) == 16);
    assert (hev_task_cio_writev (c, &iov, 1, NULL, NULL) == 16);
    assert (hev_task_cio_read_exact (c, buf, 16, NULL, NULL) == 0);
    assert (hev_task_cio_readv_exact (c, &iov, 1, NULL, NULL) == 0);
    assert (hev_task_cio_write_exact (c, buf, 16, NULL, NULL) == 16);
    assert (hev_task_cio_writev_exact (c, &iov, 1, NULL, NULL) == 16);
    assert (hev_task_cio_read_dgram (c, buf, 16, NULL, NULL, NULL) == 0);
    assert (hev_task_cio_readv_dgram (c, &iov, 1, NULL, NULL, NULL) == 0);
    assert (hev_task_cio_write_dgram (c, buf, 16, NULL, NULL, NULL) == 16);
    assert (hev_task_cio_writev_dgram (c, &iov, 1, NULL, NULL, NULL) == 16);
    assert (hev_task_cio_flush (c, NULL, NULL) == 0);

    hev_task_cio_splice (a, d, 2048, NULL, NULL);

    assert (hev_task_cio_pop (c) == b);
    assert (hev_task_cio_pop (b) == a);

    hev_object_unref (HEV_OBJECT (a));
    hev_object_unref (HEV_OBJECT (d));
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
