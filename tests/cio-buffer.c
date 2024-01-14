/*
 ============================================================================
 Name        : cio-buffer.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : CIO Buffer Test
 ============================================================================
 */

#include <assert.h>
#include <string.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>
#include <hev-task-cio-socket.h>
#include <hev-task-cio-buffer.h>
#include <hev-task-system.h>

static int fds1[2];
static int fds2[2];

static void
task_splice_entry (void *data)
{
    HevTask *task = hev_task_self ();
    HevTaskCIO *a, *b, *t;

    assert (hev_task_add_fd (task, fds1[1], POLLIN | POLLOUT) == 0);
    assert (hev_task_add_fd (task, fds2[0], POLLIN | POLLOUT) == 0);

    t = HEV_TASK_CIO (hev_task_cio_socket_new (PF_LOCAL, fds1[1]));
    assert (t != NULL);
    a = HEV_TASK_CIO (hev_task_cio_buffer_new (2048, 2048));
    assert (a != NULL);
    a = hev_task_cio_push (a, t);
    assert (a != NULL);

    t = HEV_TASK_CIO (hev_task_cio_socket_new (PF_LOCAL, fds2[0]));
    assert (t != NULL);
    b = HEV_TASK_CIO (hev_task_cio_buffer_new (2048, 2048));
    assert (b != NULL);
    b = hev_task_cio_push (b, t);
    assert (b != NULL);

    assert (hev_task_cio_get_fd (a, 0) == fds1[1]);
    assert (hev_task_cio_get_fd (a, 1) == fds1[1]);
    assert (hev_task_cio_get_fd (b, 0) == fds2[0]);
    assert (hev_task_cio_get_fd (b, 1) == fds2[0]);

    hev_task_cio_splice (a, b, 2048, NULL, NULL);

    hev_object_unref (HEV_OBJECT (a));
    hev_object_unref (HEV_OBJECT (b));
}

static void
task_entry (void *data)
{
    HevTask *task = hev_task_self ();
    HevTaskCIO *a, *b, *t;
    struct iovec iov[2];
    char buf1[64];
    char buf2[64];
    ssize_t size;
    int res;

    res = hev_task_io_socket_socketpair (PF_LOCAL, SOCK_STREAM, 0, fds1);
    assert (res == 0);
    res = hev_task_io_socket_socketpair (PF_LOCAL, SOCK_STREAM, 0, fds2);
    assert (res == 0);

    assert (hev_task_add_fd (task, fds1[0], POLLOUT) == 0);
    assert (hev_task_add_fd (task, fds2[1], POLLIN) == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_splice_entry, NULL);

    t = HEV_TASK_CIO (hev_task_cio_socket_new (PF_LOCAL, fds1[0]));
    assert (t != NULL);
    a = HEV_TASK_CIO (hev_task_cio_buffer_new (2048, 2048));
    assert (a != NULL);
    a = hev_task_cio_push (a, t);
    assert (a != NULL);

    t = HEV_TASK_CIO (hev_task_cio_socket_new (PF_LOCAL, fds2[1]));
    assert (t != NULL);
    b = HEV_TASK_CIO (hev_task_cio_buffer_new (2048, 2048));
    assert (b != NULL);
    b = hev_task_cio_push (b, t);
    assert (b != NULL);

    size = hev_task_cio_write_exact (a, buf1, 64, NULL, NULL);
    assert (size == 64);
    res = hev_task_cio_flush (a, NULL, NULL);
    assert (res == 0);
    size = hev_task_cio_read_exact (b, buf2, 64, NULL, NULL);
    assert (size == 64);
    assert (memcmp (buf1, buf2, 64) == 0);

    iov[0].iov_base = buf1;
    iov[0].iov_len = 64;
    size = hev_task_cio_writev_exact (a, iov, 1, NULL, NULL);
    assert (size == 64);
    res = hev_task_cio_flush (a, NULL, NULL);
    assert (res == 0);
    iov[0].iov_base = buf2;
    iov[0].iov_len = 64;
    size = hev_task_cio_readv_exact (b, iov, 1, NULL, NULL);
    assert (size == 64);
    assert (memcmp (buf1, buf2, 64) == 0);

    size = hev_task_cio_write_exact (a, buf1, 64, NULL, NULL);
    assert (size == 64);
    res = hev_task_cio_flush (a, NULL, NULL);
    assert (res == 0);
    for (;;) {
        size = hev_task_cio_buffer_peek (HEV_TASK_CIO_BUFFER (b), buf2, 64);
        if (size == 64)
            break;
        hev_task_yield (HEV_TASK_WAITIO);
    }
    assert (size == 64);
    assert (memcmp (buf1, buf2, 64) == 0);
    res = hev_task_cio_buffer_peekv (HEV_TASK_CIO_BUFFER (b), iov);
    assert (res == 1);
    assert (iov[0].iov_len == 64);
    assert (memcmp (buf1, buf2, 64) == 0);

    hev_object_unref (HEV_OBJECT (a));
    hev_object_unref (HEV_OBJECT (b));

    res = hev_task_io_socket_socketpair (PF_LOCAL, SOCK_DGRAM, 0, fds1);
    assert (res == 0);

    assert (hev_task_add_fd (task, fds1[0], POLLIN) == 0);
    assert (hev_task_add_fd (task, fds1[1], POLLOUT) == 0);

    t = HEV_TASK_CIO (hev_task_cio_socket_new (PF_LOCAL, fds1[0]));
    assert (t != NULL);
    a = HEV_TASK_CIO (hev_task_cio_buffer_new (2048, 2048));
    assert (a != NULL);
    a = hev_task_cio_push (a, t);
    assert (a != NULL);

    t = HEV_TASK_CIO (hev_task_cio_socket_new (PF_LOCAL, fds1[1]));
    assert (t != NULL);
    b = HEV_TASK_CIO (hev_task_cio_buffer_new (2048, 2048));
    assert (b != NULL);
    b = hev_task_cio_push (b, t);
    assert (b != NULL);

    size = hev_task_cio_write_dgram (a, buf1, 64, NULL, NULL, NULL);
    assert (size == 64);
    size = hev_task_cio_read_dgram (b, buf2, 64, NULL, NULL, NULL);
    assert (size == 64);
    assert (memcmp (buf1, buf2, 64) == 0);

    iov[0].iov_base = buf1;
    iov[0].iov_len = 64;
    size = hev_task_cio_writev_dgram (a, iov, 1, NULL, NULL, NULL);
    assert (size == 64);
    iov[0].iov_base = buf2;
    iov[0].iov_len = 64;
    size = hev_task_cio_readv_dgram (b, iov, 1, NULL, NULL, NULL);
    assert (size == 64);
    assert (memcmp (buf1, buf2, 64) == 0);

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
