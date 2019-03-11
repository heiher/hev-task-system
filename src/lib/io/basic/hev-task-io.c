/*
 ============================================================================
 Name        : hev-task-io.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Task I/O operations
 ============================================================================
 */

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>

#include "kern/task/hev-task.h"
#include "hev-task-io.h"
#include "lib/io/buffer/hev-circular-buffer.h"

int
hev_task_io_open (const char *pathname, int flags, ...)
{
    flags |= O_NONBLOCK;

    if (O_CREAT & flags) {
        int fd;
        va_list ap;

        va_start (ap, flags);
        fd = open (pathname, flags, va_arg (ap, int));
        va_end (ap);

        return fd;
    }

    return open (pathname, flags);
}

int
hev_task_io_creat (const char *pathname, mode_t mode)
{
    return hev_task_io_open (pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int
hev_task_io_openat (int dirfd, const char *pathname, int flags, ...)
{
    flags |= O_NONBLOCK;

    if (O_CREAT & flags) {
        int fd;
        va_list ap;

        va_start (ap, flags);
        fd = openat (dirfd, pathname, flags, va_arg (ap, int));
        va_end (ap);

        return fd;
    }

    return openat (dirfd, pathname, flags);
}

int
hev_task_io_dup (int oldfd)
{
    int newfd;
    int nonblock = 1;

    newfd = dup (oldfd);
    if (0 > newfd)
        return -1;

    if (0 > ioctl (newfd, FIONBIO, (char *)&nonblock)) {
        close (newfd);
        return -2;
    }

    return newfd;
}

int
hev_task_io_dup2 (int oldfd, int newfd)
{
    int nonblock = 1;

    newfd = dup2 (oldfd, newfd);
    if (0 > newfd)
        return -1;

    if (0 > ioctl (newfd, FIONBIO, (char *)&nonblock)) {
        close (newfd);
        return -2;
    }

    return newfd;
}

ssize_t
hev_task_io_read (int fd, void *buf, size_t count, HevTaskIOYielder yielder,
                  void *yielder_data)
{
    ssize_t s;

retry:
    s = read (fd, buf, count);
    if (s == -1 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;
}

ssize_t
hev_task_io_readv (int fd, const struct iovec *iov, int iovcnt,
                   HevTaskIOYielder yielder, void *yielder_data)
{
    ssize_t s;

retry:
    s = readv (fd, iov, iovcnt);
    if (s == -1 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;
}

ssize_t
hev_task_io_write (int fd, const void *buf, size_t count,
                   HevTaskIOYielder yielder, void *yielder_data)
{
    ssize_t s;

retry:
    s = write (fd, buf, count);
    if (s == -1 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;
}

ssize_t
hev_task_io_writev (int fd, const struct iovec *iov, int iovcnt,
                    HevTaskIOYielder yielder, void *yielder_data)
{
    ssize_t s;

retry:
    s = writev (fd, iov, iovcnt);
    if (s == -1 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;
}

static int
task_io_splice (int fd_in, int fd_out, HevCircularBuffer *buf)
{
    struct iovec iov[2];
    int res = 1, iovc;

    iovc = hev_circular_buffer_writing (buf, iov);
    if (iovc) {
        ssize_t s = readv (fd_in, iov, iovc);
        if (0 >= s) {
            if ((0 > s) && (EAGAIN == errno))
                res = 0;
            else
                res = -1;
        } else {
            hev_circular_buffer_write_finish (buf, s);
        }
    }

    iovc = hev_circular_buffer_reading (buf, iov);
    if (iovc) {
        ssize_t s = writev (fd_out, iov, iovc);
        if (0 >= s) {
            if ((0 > s) && (EAGAIN == errno))
                res = 0;
            else
                res = -1;
        } else {
            res = 1;
            hev_circular_buffer_read_finish (buf, s);
        }
    }

    return res;
}

void
hev_task_io_splice (int fd_a_i, int fd_a_o, int fd_b_i, int fd_b_o,
                    size_t buf_size, HevTaskIOYielder yielder,
                    void *yielder_data)
{
    HevCircularBuffer *buf_f;
    HevCircularBuffer *buf_b;

    buf_f = hev_circular_buffer_new (buf_size);
    if (!buf_f)
        return;
    buf_b = hev_circular_buffer_new (buf_size);
    if (!buf_b)
        goto err;

    for (;;) {
        HevTaskYieldType type = 0;

        if (buf_f) {
            int ret = task_io_splice (fd_a_i, fd_b_o, buf_f);
            if (0 >= ret) {
                /* backward closed, quit */
                if (!buf_b)
                    break;
                if (0 > ret) { /* error */
                    /* forward error or closed, mark to skip */
                    hev_circular_buffer_unref (buf_f);
                    buf_f = NULL;
                } else { /* no data */
                    type++;
                }
            }
        }

        if (buf_b) {
            int ret = task_io_splice (fd_b_i, fd_a_o, buf_b);
            if (0 >= ret) {
                /* forward closed, quit */
                if (!buf_f)
                    break;
                if (0 > ret) { /* error */
                    /* backward error or closed, mark to skip */
                    hev_circular_buffer_unref (buf_b);
                    buf_b = NULL;
                } else { /* no data */
                    type++;
                }
            }
        }

        if (yielder) {
            if (yielder (type, yielder_data))
                break;
        } else {
            hev_task_yield (type);
        }
    }

    if (buf_b)
        hev_circular_buffer_unref (buf_b);
err:
    if (buf_f)
        hev_circular_buffer_unref (buf_f);
}
