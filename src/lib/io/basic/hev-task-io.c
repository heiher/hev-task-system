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
#include <fcntl.h>
#include <stdarg.h>

#include "kern/task/hev-task.h"
#include "hev-task-io.h"

int
hev_task_io_open (const char *pathname, int flags, ...)
{
    flags |= O_NONBLOCK;

    if (O_CREAT & flags) {
        int fd;
        va_list ap;

        va_start (ap, flags);
        fd = open (pathname, flags, va_arg (ap, mode_t));
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
task_io_splice (int fd_in, int fd_out, void *buf, size_t len, size_t *w_off,
                size_t *w_left)
{
    ssize_t s;

    if (*w_left == 0) {
        s = read (fd_in, buf, len);
        if (s == -1) {
            if (errno == EAGAIN)
                return 0;
            else
                return -1;
        } else if (s == 0) {
            return -1;
        } else {
            *w_off = 0;
            *w_left = s;
        }
    }

    s = write (fd_out, buf + *w_off, *w_left);
    if (s == -1) {
        if (errno == EAGAIN)
            return 0;
        else
            return -1;
    } else if (s == 0) {
        return -1;
    } else {
        *w_off += s;
        *w_left -= s;
    }

    return *w_off;
}

void
hev_task_io_splice (int fd_a_i, int fd_a_o, int fd_b_i, int fd_b_o,
                    size_t buf_size, HevTaskIOYielder yielder,
                    void *yielder_data)
{
    int splice_f = 1, splice_b = 1;
    size_t w_off_f = 0, w_off_b = 0;
    size_t w_left_f = 0, w_left_b = 0;
    unsigned char buf_f[buf_size];
    unsigned char buf_b[buf_size];

    for (;;) {
        int no_data = 0;
        HevTaskYieldType type;

        if (splice_f) {
            int ret;

            ret = task_io_splice (fd_a_i, fd_b_o, buf_f, buf_size, &w_off_f,
                                  &w_left_f);
            if (ret == 0) { /* no data */
                /* forward no data and backward closed, quit */
                if (!splice_b)
                    break;
                no_data++;
            } else if (ret == -1) { /* error */
                /* forward error and backward closed, quit */
                if (!splice_b)
                    break;
                /* forward error or closed, mark to skip */
                splice_f = 0;
            }
        }

        if (splice_b) {
            int ret;

            ret = task_io_splice (fd_b_i, fd_a_o, buf_b, buf_size, &w_off_b,
                                  &w_left_b);
            if (ret == 0) { /* no data */
                /* backward no data and forward closed, quit */
                if (!splice_f)
                    break;
                no_data++;
            } else if (ret == -1) { /* error */
                /* backward error and forward closed, quit */
                if (!splice_f)
                    break;
                /* backward error or closed, mark to skip */
                splice_b = 0;
            }
        }

        /* single direction no data, goto yield.
		 * double direction no data, goto waitio.
		 */
        type = (no_data < 2) ? HEV_TASK_YIELD : HEV_TASK_WAITIO;
        if (yielder) {
            if (yielder (type, yielder_data))
                break;
        } else {
            hev_task_yield (type);
        }
    }
}
